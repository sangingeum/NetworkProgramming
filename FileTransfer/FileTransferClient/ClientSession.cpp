#include "ClientSession.hpp"

ClientSession::ClientSession(std::shared_ptr<tcp::socket> socket)
: m_socket(socket), m_parser(), m_curPath(std::filesystem::current_path())
{
    m_parser.setHandler(file_transfer::Message::kFileList, [this](const file_transfer::Message& message) { handleFileList(message.file_list()); });
    m_parser.setHandler(file_transfer::Message::kFileInfo, [this](const file_transfer::Message& message) { handleFileInfo(message.file_info()); });
    m_parser.setHandler(file_transfer::Message::kFileChunk, [this](const file_transfer::Message& message) { handleFileChunk(message.file_chunk()); });
    m_parser.setHandler(file_transfer::Message::kFileTransferComplete, [this](const file_transfer::Message& message) { handleFileTransferCompletion(message.file_transfer_complete()); });
    m_parser.setHandler(file_transfer::Message::kError, [this](const file_transfer::Message& message) { handleError(message.error()); });
    std::cout << "Client connected to server from " << m_socket->remote_endpoint().address().to_string() << ":" << m_socket->remote_endpoint().port() << '\n';
}

void ClientSession::start(){
    auto buffer = std::make_shared<ReadBuffer>();
    m_socket->async_read_some(asio::buffer(buffer->data(), buffer->size()),
    std::bind(&ClientSession::handleRead, shared_from_this(), buffer, std::placeholders::_1, std::placeholders::_2));
    sendFileListRequest();
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

void ClientSession::sendAcknowledgement(bool success) {
	file_transfer::Message message;
	message.mutable_client_acknowledgement()->set_success(success);
	std::string data = Packager::packageMessage(message);
	asio::async_write(*m_socket, asio::buffer(data), [](const asio::error_code& code, size_t bytesTransferred){
		if(!code){
			std::cout << "Succecssfully sent client status to the server.\n";
		}}
	);
}

void ClientSession::sendReady()
{
	file_transfer::Message message;
	message.mutable_client_ready();
	std::string data = Packager::packageMessage(message);
	// TODO
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
	// TODO
	
}
void ClientSession::handleFileChunk(const file_transfer::FileChunk& chunk){
	// write to file

}
void ClientSession::handleFileTransferCompletion(const file_transfer::FileTransferComplete& complete){
	// check file name collision
	// change file name
	// close file
}
void ClientSession::handleError(const file_transfer::Error& error){
	// stop reading and close file
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