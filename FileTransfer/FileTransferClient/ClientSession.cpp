#include "ClientSession.hpp"

ClientSession::ClientSession(std::shared_ptr<tcp::socket> socket)
: m_socket(socket), m_parser(), m_curPath(std::filesystem::current_path())
{
    m_parser.setHandler(file_transfer::Message::kFileList, [this](const file_transfer::Message& message) { handleFileList(message.file_list()); });
    m_parser.setHandler(file_transfer::Message::kFileInfo, [this](const file_transfer::Message& message) { handleFileInfo(message.file_info()); });
    m_parser.setHandler(file_transfer::Message::kFileChunk, [this](const file_transfer::Message& message) { handleFileChunk(message.file_chunk()); });
    m_parser.setHandler(file_transfer::Message::kFileTransferComplete, [this](const file_transfer::Message& message) { handleFileTransferComplete(message.file_transfer_complete()); });
    m_parser.setHandler(file_transfer::Message::kError, [this](const file_transfer::Message& message) { handleError(message.error()); });
	m_parser.setHandler(file_transfer::Message::kFileTransferError, [this](const file_transfer::Message& message) { handleFileTransferError(message.file_transfer_error()); });
    std::cout << "Client connected to server from " << m_socket->remote_endpoint().address().to_string() << ":" << m_socket->remote_endpoint().port() << '\n';
}

void ClientSession::start(){
    auto buffer = std::make_shared<ReadBuffer>();
    m_socket->async_read_some(asio::buffer(buffer->data(), buffer->size()),
    std::bind(&ClientSession::handleRead, shared_from_this(), buffer, std::placeholders::_1, std::placeholders::_2));
}

void ClientSession::sendFileTransferRequest(std::string_view fileName){
	//TODO: Check if the files already exists
	file_transfer::Message message;
	message.mutable_file_transfer_request()->set_name(fileName.data());
	std::string data = Packager::packageMessage(message);
	asio::async_write(*m_socket, asio::buffer(data.data(), data.size()), [](const asio::error_code& code, size_t bytesTransferred){
		if(!code){
			std::cout << "Succecssfully sent file tranfer request to the server.\n";
		}
	});
}

void ClientSession::sendFileListRequest(){
	file_transfer::Message message;
	message.mutable_file_list_request();
	std::string data = Packager::packageMessage(message);
	asio::async_write(*m_socket, asio::buffer(data.data(), data.size()), [](const asio::error_code& code, size_t bytesTransferred){
		if(!code){
			std::cout << "Succecssfully sent file list request to the server.\n";
			std::cout << "bytesTransferred: " << bytesTransferred << '\n';
		}}
	);
}

void ClientSession::sendReady(uint32_t fileIdentifier)
{
	file_transfer::Message message;
	message.mutable_client_ready()->set_id(fileIdentifier);
	std::string data = Packager::packageMessage(message);
	asio::async_write(*m_socket, asio::buffer(data.data(), data.size()), [](const asio::error_code& code, size_t bytesTransferred) {
		if (!code) {
			std::cout << "Successfully sent client ready message to the server.\n";
		}
		});
}

void ClientSession::resetFileTransferState(uint32_t fileIdentifier){
	m_activeFileTransfers.erase(fileIdentifier);
	m_activeOriginalFileNames.erase(fileIdentifier);
	m_activeTempFileNames.erase(fileIdentifier);
}

void ClientSession::sendFileTransferError(file_transfer::ErrorCode code, uint32_t fileIdentifier, std::string_view errorMessage) {
	file_transfer::Message message;
	auto* error = message.mutable_file_transfer_error();
	error->set_id(fileIdentifier);
	error->set_code(code);
	error->set_message(errorMessage.data());
	std::string data = Packager::packageMessage(message);
	asio::async_write(*m_socket, asio::buffer(data.data(), data.size()), [](const asio::error_code& code, size_t bytesTransferred) {
		if (!code) {
			std::cout << "Successfully sent file transfer error to the server.\n";
		}
		});

}

void ClientSession::handleRead(std::shared_ptr<ReadBuffer> buffer, const asio::error_code& code, size_t bytesTransferred) {
	if(!code){
		m_parser.appendDataAndParse(buffer->data(), bytesTransferred);
		m_socket->async_read_some(asio::buffer(buffer->data(), buffer->size()),
		std::bind(&ClientSession::handleRead, shared_from_this(), buffer, std::placeholders::_1, std::placeholders::_2));
	}
	else{
		std::cerr << "Error on receive: " << code.message() << '\n';
	}
}


// Message handlers

void ClientSession::handleFileList(const file_transfer::FileList& list) {
	std::cout << "Received file list from the server:\n";
	for (const auto& file : list.files()) {
		std::cout << file.name() << ", " << file.size() << '\n';
	}
}

void ClientSession::handleFileInfo(const file_transfer::FileInfo& info){
	std::cout << "Start downloading file: " << info.name() << " (" << info.size() << " bytes)\n";
	// make a random name
	std::string filename = generateRandomUniqueFilename();
	// open file
	auto outFile = std::make_shared<std::ofstream>(filename, std::ios::binary);
	if (!outFile->is_open() || !outFile->good()) {
		std::cerr << "Failed to open file for writing: " << filename << '\n';
		sendFileTransferError(file_transfer::ErrorCode::INTERNAL, info.id(), "Failed to open file for writing.");
		return;
	}
	m_activeFileTransfers[info.id()] = outFile;
	m_activeOriginalFileNames[info.id()] = info.name();
	m_activeTempFileNames[info.id()] = filename;
	sendReady(info.id());
}
void ClientSession::handleFileChunk(const file_transfer::FileChunk& chunk){
	auto activeFile = m_activeFileTransfers[chunk.id()];
	if (!activeFile) {
		std::cerr << "No active file transfer found for ID: " << chunk.id() << '\n';
		sendFileTransferError(file_transfer::ErrorCode::INTERNAL, chunk.id(), "No active file transfer found for the given ID.");
		return;
	}
	if(!activeFile->good()){
		std::cerr << "Error writing to file for ID: " << chunk.id() << '\n';
		sendFileTransferError(file_transfer::ErrorCode::INTERNAL, chunk.id(), "Error writing to file.");
		return;
	}
	activeFile->write(chunk.data().data(), chunk.data().size());
}
void ClientSession::handleFileTransferComplete(const file_transfer::FileTransferComplete& complete){
	auto activeFile = m_activeFileTransfers[complete.id()];
	if (!activeFile) {
		std::cerr << "No active file transfer found for ID: " << complete.id() << '\n';
		sendFileTransferError(file_transfer::ErrorCode::INTERNAL, complete.id(), "No active file transfer found for the given ID.");
		return;
	}
	activeFile->close();
	// Change the filename to the original name if needed
	std::string originalName = m_activeOriginalFileNames[complete.id()];
	std::string tempName = m_activeTempFileNames[complete.id()];
	if (!originalName.empty()) {
		std::filesystem::path newPath = m_curPath / originalName;
		if (std::filesystem::exists(newPath)) {
			// add a number suffix to avoid overwriting
			std::string extension = newPath.extension().string();
			std::string originalNameWithoutExt = newPath.stem().string();
			size_t counter = 1;
			while(counter < 10000 && std::filesystem::exists(newPath)){
				// Question: Does this preserve the file extension?
				newPath = m_curPath / (originalNameWithoutExt  + " (" + std::to_string(counter) + ")" + extension);
				++counter;
			}
			if(counter == 1000){
				std::cerr << "Failed to rename file: " << originalName << " due to existing files.\n";
			}
		}
		std::filesystem::rename(m_curPath / tempName, newPath);
		std::cout << "Renamed file to: " << newPath << '\n';
	}
	// Reset state
	resetFileTransferState(complete.id());
	std::cout << "File transfer complete for ID: " << complete.id() << '\n';
	
}
void ClientSession::handleError(const file_transfer::Error& error){
	std::cerr << "Received error from server: " << error.message() << " (code: " << static_cast<int>(error.code()) << ")\n";
}


void ClientSession::handleFileTransferError(const file_transfer::FileTransferError& error){
	// stop reading and close file
	resetFileTransferState(error.id());
	std::cerr << "Received file transfer error from server for ID " << error.id() << ": " << error.message() << '\n';
}

// File operations
std::string ClientSession::generateRandomUniqueFilename(){
	static std::string_view textPool =
		"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	static std::mt19937 rng{ std::random_device{}() };
	static std::uniform_int_distribution<std::size_t> dist(0, textPool.size() - 1);

	std::string filename;
	filename.reserve(13); // Reserve space for 13 characters
	do {
		filename.clear();
		for (size_t i = 0; i < 12; ++i) { // Generate a 12-character random string
			filename += textPool[dist(rng)];
		}
		filename += ".dat"; // Add a file extension
	} while (std::filesystem::exists(m_curPath / filename)); // Ensure the filename is unique

	return filename;
}