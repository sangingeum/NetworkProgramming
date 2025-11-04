#include "ServerSession.hpp"

ServerSession::ServerSession(std::shared_ptr<tcp::socket> socket)
    : m_socket(socket), m_parser{}, m_curPath(std::filesystem::current_path()), m_identifierCounter(0)
{
    m_parser.setHandler(file_transfer::Message::kFileListRequest, [this](const file_transfer::Message& msg){ {
        handleFileListRequest(msg.file_list_request());
    }});
    m_parser.setHandler(file_transfer::Message::kFileTransferRequest, [this](const file_transfer::Message& msg){ {
        handleFileTransferRequest(msg.file_transfer_request());
    }});
    m_parser.setHandler(file_transfer::Message::kClientReady, [this](const file_transfer::Message& msg){ {
        handleClientReady(msg.client_ready());
    }});
    m_parser.setHandler(file_transfer::Message::kError, [this](const file_transfer::Message& msg){ {
        handleError(msg.error());
    }});
    m_parser.setHandler(file_transfer::Message::kFileTransferError, [this](const file_transfer::Message& msg){ {
        handleFileTransferError(msg.file_transfer_error());
    }});
}

void ServerSession::start(){
	auto data = std::make_shared<ReadBuffer>();
	m_socket->async_read_some(asio::buffer(data->data(), data->size()), std::bind(&ServerSession::handleRead, shared_from_this(), data, std::placeholders::_1, std::placeholders::_2));
}

void ServerSession::handleRead(std::shared_ptr<ReadBuffer> data, const asio::error_code& code, size_t bytesTransferred){
	if (!code) {
		std::cout << "Received data from the client.\n";
		std::cout << "bytesTransferred: " << bytesTransferred << '\n';	
		m_parser.appendDataAndParse(data->data(), bytesTransferred);
		m_socket->async_read_some(asio::buffer(data->data(), data->size()), std::bind(&ServerSession::handleRead, shared_from_this(), data, std::placeholders::_1, std::placeholders::_2));
	}
	else{
		std::cout << "Error on receive: " << code.message() << '\n';
	}
}


void ServerSession::handleFileListRequest(const file_transfer::FileListRequest& request){
	file_transfer::Message message;
	file_transfer::FileList* fileList = message.mutable_file_list();
	for (const auto& entry : std::filesystem::directory_iterator(m_curPath)) {
		if (entry.is_regular_file()) {
			auto* fileInfo = fileList->add_files();
			fileInfo->set_name(entry.path().filename().string());
			fileInfo->set_size(entry.file_size());
		}
	}
	std::string serializedData = Packager::packageMessage(message);
	asio::async_write(*m_socket, asio::buffer(serializedData.data(), serializedData.size()), [](const asio::error_code& code, size_t bytesTransferred) {
		if (!code) {
			std::cout << "Sent file list to client.\n";
		}
		});
}
void ServerSession::handleFileTransferRequest(const file_transfer::FileTransferRequest& request){
	std::string fileName = request.name();
	std::filesystem::path filePath = m_curPath / fileName;
	// Check if file exists
	if (!std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath)) {
		std::cout << "Requested file does not exist: " << filePath << '\n';
		sendError(file_transfer::ErrorCode::NOT_FOUND);
		return;
	}
	std::cout << "Getting ready to send file: " << filePath << '\n';
	// open file and check
	auto fileStream = std::make_shared<std::ifstream>(filePath, std::ios::binary);
	if (!(fileStream->is_open() && fileStream->good())) {
		std::cout << "Failed to open file: " << filePath << '\n';
		sendError(file_transfer::ErrorCode::NOT_FOUND);
		return;
	}
	uint32_t currentId = getNextIdentifier();
	m_activeFileTransfers[currentId] = std::move(fileStream);
	sendFileInfo(filePath, currentId);
	// Now wait for ClientReady message before sending the file
}
void ServerSession::handleClientReady(const file_transfer::ClientReady& ready)
{
	sendFile(ready.id());
}

void ServerSession::handleError(const file_transfer::Error& error){
	switch(error.code()){
		case file_transfer::ErrorCode::NOT_FOUND:{
			std::cout << "Client reported error: NOT_FOUND\n";
			break;
		}
		case file_transfer::ErrorCode::INTERNAL:{
			std::cout << "Client reported error: INTERNAL\n";
			break;	
		}
		default:{
			std::cout << "Client reported error: UNKNOWN\n";
			break;
		}
	}
}

void ServerSession::handleFileTransferError(const file_transfer::FileTransferError& error){
	uint32_t fileId = error.id();
	auto it = m_activeFileTransfers.find(fileId);
	if (it != m_activeFileTransfers.end()) {
		std::cout << "Client reported file transfer error for ID " << fileId << ": " << error.message() << '\n';
		// Clean up the active file transfer
		m_activeFileTransfers.erase(it);
	} else {
		std::cout << "Received file transfer error for unknown ID " << fileId << '\n';
	}
}

// Send

void ServerSession::sendFileHelper(uint32_t fileIdentifier, uint32_t chunkId){
	constexpr static size_t maxChunkSize = 1024 * 16; // 16KB
	auto fileStream = m_activeFileTransfers[fileIdentifier];

	if(!fileStream){
		// This could happend if a file transfer error was sent and the state reset
		std::cout << "No active file transfer found for ID: " << fileIdentifier << '\n';
		sendFileTransferError(file_transfer::ErrorCode::INTERNAL, fileIdentifier, "No active file transfer found.");
		return;
	}
	if(!(fileStream->is_open() && fileStream->good())){
		std::cout << "File status not good" << '\n';
		sendFileTransferError(file_transfer::ErrorCode::INTERNAL, fileIdentifier, "File stream is not in a good state.");
		return;
	}
	if(chunkId == 0){
		std::cout << "Starting to send file..." << '\n';
		fileStream->seekg(0, std::ios::beg);
	}
	if(fileStream->eof()){
		std::cout << "Finished sending file." << '\n';
		sendFileTransferComplete(fileIdentifier);
		return;
	}
	auto buffer = std::make_shared<std::vector<std::byte>>(maxChunkSize);
	fileStream->read(reinterpret_cast<char*>(buffer->data()), maxChunkSize);
	std::streamsize bytesRead = fileStream->gcount();

	if (bytesRead > 0) {
		file_transfer::Message message;
		auto* fileChunk = message.mutable_file_chunk();
		fileChunk->set_id(fileIdentifier);
		fileChunk->set_data(buffer->data(), bytesRead);
		std::string serializedData = Packager::packageMessage(message);
		asio::async_write(*m_socket, asio::buffer(serializedData.data(), serializedData.size()), [this, fileStream, fileIdentifier, chunkId](const asio::error_code& code, size_t bytesTransferred) {
			if (!code) {
				std::cout << "Sent chunk " << chunkId << " to client.\n";
				sendFileHelper(fileIdentifier, chunkId + 1);
			} else {
				std::cout << "Failed to send chunk " << chunkId << " to client. Error: " << code.message() << '\n';
				sendFileTransferError(file_transfer::ErrorCode::INTERNAL, fileIdentifier, "Failed to send file chunk.");
			}
			});
	} else {
		if (fileStream->eof()) {
			std::cout << "Finished sending file." << '\n';
			sendFileTransferComplete(fileIdentifier);
		} else {
			std::cout << "Failed to read from file stream." << '\n';
			sendFileTransferError(file_transfer::ErrorCode::INTERNAL, fileIdentifier, "Failed to read from file stream.");
		}
	}
}
void ServerSession::sendFile(uint32_t fileIdentifier){
	auto fileStream = m_activeFileTransfers[fileIdentifier];
	if (!fileStream) {
		std::cout << "No active file transfer found for ID: " << fileIdentifier << '\n';
		sendFileTransferError(file_transfer::ErrorCode::INTERNAL, fileIdentifier, "No active file transfer found.");
		return;
	}
	fileStream->seekg(0, std::ios::beg);
	// Start sending the file
	sendFileHelper(fileIdentifier, 0u);
}
void ServerSession::sendError(file_transfer::ErrorCode code){
	file_transfer::Message message;
	message.mutable_error()->set_code(code);
	std::string serializedData = Packager::packageMessage(message);
	asio::async_write(*m_socket, asio::buffer(serializedData.data(), serializedData.size()), [code](const asio::error_code& code, size_t bytesTransferred) {
		if (!code) {
			std::cout << "Sent error to client. Code: " << code << '\n';
		}
		});
}

void ServerSession::sendFileTransferError(file_transfer::ErrorCode code, uint32_t fileIdentifier, std::string_view errorMessage){
	// Send error
	file_transfer::Message message;
	file_transfer::FileTransferError* errorMsg = message.mutable_file_transfer_error();
	errorMsg->set_id(fileIdentifier);
	errorMsg->set_code(code);
	errorMsg->set_message(errorMessage.data(), errorMessage.size());
	std::string serializedData = Packager::packageMessage(message);
	asio::async_write(*m_socket, asio::buffer(serializedData.data(), serializedData.size()), [code, fileIdentifier](const asio::error_code& ec, size_t bytesTransferred) {
		if (!ec) {
			std::cout << "Sent file transfer error to client. Code: " << code << ", ID: " << fileIdentifier << '\n';
		}
		});
	// Reset state related to the file transfer
	m_activeFileTransfers.erase(fileIdentifier); 
}

void ServerSession::sendFileTransferComplete(uint32_t fileIdentifier){
	file_transfer::Message message;
	message.mutable_file_transfer_complete()->set_id(fileIdentifier);
	std::string serializedData = Packager::packageMessage(message);
	asio::async_write(*m_socket, asio::buffer(serializedData.data(), serializedData.size()), [](const asio::error_code& code, size_t bytesTransferred) {
		if (!code) {
			std::cout << "Sent file transfer complete to client.\n";
		}
		});
}

void ServerSession::sendFileInfo(const std::filesystem::path& filePath, uint32_t fileIdentifier){
	// Prepare FileInfo message
	file_transfer::Message message;
	file_transfer::FileInfo *fileInfo = message.mutable_file_info();
	fileInfo->set_id(fileIdentifier);
	fileInfo->set_name(filePath.filename().string());
	fileInfo->set_size(std::filesystem::file_size(filePath));
	// Send FileInfo message
	std::string serializedData = Packager::packageMessage(message);
	asio::async_write(*m_socket, asio::buffer(serializedData.data(), serializedData.size()), [](const asio::error_code& code, size_t bytesTransferred) {
		if (!code) {
			std::cout << "Sent file info to client.\n";
		}
		});
}