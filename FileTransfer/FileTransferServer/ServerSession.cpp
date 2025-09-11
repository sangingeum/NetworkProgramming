#include "ServerSession.hpp"

ServerSession::ServerSession(std::shared_ptr<tcp::socket> socket)
    : m_socket(socket), m_parser{}, m_curPath(std::filesystem::current_path()) 
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
    m_parser.setHandler(file_transfer::Message::kClientAcknowledgement, [this](const file_transfer::Message& msg){ {
        handleClientAcknowledgement(msg.client_acknowledgement());
    }});
    m_parser.setHandler(file_transfer::Message::kError, [this](const file_transfer::Message& msg){ {
        handleError(msg.error());
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
	std::cout << "Getting ready to send file: " << filePath << '\n';
	sendFile(filePath);
}
void ServerSession::handleClientReady(const file_transfer::ClientReady& ready)
{
	// TODO: RESET

}
void ServerSession::handleClientAcknowledgement(const file_transfer::ClientAcknowledgement& ack) {
	bool success = ack.success();
	success ? std::cout << "Client reported success.\n" : std::cout << "Client reported failure.\n";
	if(!success){
		// TODO: RESET
	}
}
void ServerSession::handleError(const file_transfer::Error& error){
	switch(error.code()){
		case file_transfer::Error_Code::Error_Code_NOT_FOUND:{
			std::cout << "Client reported error: NOT_FOUND\n";
			break;
		}
		case file_transfer::Error_Code::Error_Code_INTERNAL:{
			std::cout << "Client reported error: INTERNAL\n";
			break;	
		}
		default:{
			std::cout << "Client reported error: UNKNOWN\n";
			break;
		}
	}
	// TODO: RESET
}


// Send
void ServerSession::sendFile(std::filesystem::path filePath){
	auto fileStream = std::make_shared<std::ifstream>(filePath, std::ios::binary);
	if(!(fileStream->is_open() && fileStream->good())){
		std::cout << "Failed to open file: " << filePath << '\n';
		sendError(file_transfer::Error::NOT_FOUND);
		return;
	}
	fileStream->seekg(0, std::ios::beg);
	sendFileInfo(filePath, [this, fileStream](){ sendFileHelper(fileStream, 0);});
}
void ServerSession::sendFileHelper(std::shared_ptr<std::ifstream> fileStream, uint32_t chunkId){
	constexpr static size_t maxChunkSize = 1024 * 16; // 16KB
	if(!(fileStream->is_open() && fileStream->good())){
		std::cout << "File status not good" << '\n';
		sendError(file_transfer::Error::INTERNAL);
		return;
	}
	if(fileStream->eof()){
		std::cout << "Finished sending file." << '\n';
		sendFileTransferComplete();
		return;
	}
	if(chunkId == 0){
		std::cout << "Starting to send file..." << '\n';
	}
	auto buffer = std::make_shared<std::vector<std::byte>>(maxChunkSize);
	fileStream->read(reinterpret_cast<char*>(buffer->data()), maxChunkSize);
	std::streamsize bytesRead = fileStream->gcount();
	file_transfer::Message message;
	message.mutable_file_chunk()->set_data(buffer->data(), bytesRead);
	std::string serializedData = Packager::packageMessage(message);
	asio::async_write(*m_socket, asio::buffer(serializedData.data(), serializedData.size()), 	[this, fileStream, chunkId](const asio::error_code& code, size_t bytesTransferred) {
		if (!code) {
			std::cout << "Sent chunk " << chunkId << " to client.\n";
			sendFileHelper(fileStream, chunkId + 1);
		} else {
			std::cout << "Failed to send chunk " << chunkId << " to client. Error: " << code.message() << '\n';
			sendError(file_transfer::Error::INTERNAL);
		}
		});
}

void ServerSession::sendError(file_transfer::Error::Code code){
	file_transfer::Message message;
	message.mutable_error()->set_code(code);
	std::string serializedData = Packager::packageMessage(message);
	asio::async_write(*m_socket, asio::buffer(serializedData.data(), serializedData.size()), [code](const asio::error_code& code, size_t bytesTransferred) {
		if (!code) {
			std::cout << "Sent error to client. Code: " << code << '\n';
		}
		});
}

void ServerSession::sendFileTransferComplete(){
	file_transfer::Message message;
	message.mutable_file_transfer_complete();
	std::string serializedData = Packager::packageMessage(message);
	asio::async_write(*m_socket, asio::buffer(serializedData.data(), serializedData.size()), [](const asio::error_code& code, size_t bytesTransferred) {
		if (!code) {
			std::cout << "Sent file transfer complete to client.\n";
		}
		});
}

void ServerSession::sendFileInfo(const std::filesystem::path& filePath, std::function<void()> onSent){
	file_transfer::Message message;
	auto* fileInfo = message.mutable_file_info();
	fileInfo->set_name(filePath.filename().string());
	fileInfo->set_size(std::filesystem::file_size(filePath));
	std::string serializedData = Packager::packageMessage(message);
	asio::async_write(*m_socket, asio::buffer(serializedData.data(), serializedData.size()), [onSent](const asio::error_code& code, size_t bytesTransferred) {
		if (!code) {
			std::cout << "Sent file info to client.\n";
			onSent();
		}
		});
}