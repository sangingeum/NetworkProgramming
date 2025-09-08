#include "FileTransferServer.hpp"
#include <iostream>

FileTransferServer::FileTransferServer(asio::io_context& context, unsigned port)
	: m_context(context), m_endpoint(tcp::v4(), port), m_curPath(std::filesystem::current_path())
{}

void FileTransferServer::start() {
	auto acceptor = std::make_shared<tcp::acceptor>(m_context, m_endpoint);
	acceptClient(acceptor);
}

void FileTransferServer::acceptClient(std::shared_ptr<tcp::acceptor> acceptor) {
	std::cout << "Wating for a client...\n";
    acceptor->async_accept([this, acceptor](const asio::error_code& code, tcp::socket peer) {
        if (!code) {
            std::cout << "Accepted a new client connection.\n";
            handleClient(std::make_shared<tcp::socket>(std::move(peer)));
        }
        acceptClient(acceptor);
    });
}

void FileTransferServer::handleClient(std::shared_ptr<tcp::socket> socket) {
	std::shared_ptr<std::array<std::byte, 1024>> data = std::make_shared<std::array<std::byte, 1024>>();
	socket->async_read_some(asio::buffer(data->data(), data->size()), std::bind(&FileTransferServer::handleRead, this, socket, data, std::placeholders::_1, std::placeholders::_2));
}

void FileTransferServer::handleRead(std::shared_ptr<tcp::socket> socket, std::shared_ptr<std::array<std::byte, 1024>> data, const asio::error_code& code, size_t bytesTransferred){
	
	if (!code) {
		std::cout << "Received data from the client.\n";
		std::cout << "bytesTransferred: " << bytesTransferred << '\n';	
		file_transfer::Message message;
		message.ParseFromArray(data->data(), static_cast<int>(bytesTransferred));
		switch (message.content_case()) {
		case file_transfer::Message::kFileListRequest:
		{
			handleFileListRequest(socket);
			break;
		}
		case file_transfer::Message::kFileTransferRequest:
		{
			handlerFileTransferRequest(socket, message.file_transfer_request());
			break;
		}
		case file_transfer::Message::kClientStatus:
		{
			handlerClientStatus(socket, message.client_status());
			break;
		}
		case file_transfer::Message::kError:
		{
			handlerError(socket, message.error());
			break;
		}
		default:
			std::cout << "Unknown message type received from client.\n";
			break;
		}
		socket->async_read_some(asio::buffer(data->data(), data->size()), std::bind(&FileTransferServer::handleRead, this, socket, data, std::placeholders::_1, std::placeholders::_2));
	}
	else{
		std::cout << "Error on receive: " << code.message() << '\n';
	}
}


void FileTransferServer::handleFileListRequest(std::shared_ptr<tcp::socket> socket){
	file_transfer::Message message;
	file_transfer::FileList* fileList = message.mutable_file_list();
	for (const auto& entry : std::filesystem::directory_iterator(m_curPath)) {
		if (entry.is_regular_file()) {
			auto* fileInfo = fileList->add_files();
			fileInfo->set_name(entry.path().filename().string());
			fileInfo->set_size(entry.file_size());
		}
	}
	std::string serializedData = message.SerializeAsString();
	asio::async_write(*socket, asio::buffer(serializedData.data(), serializedData.size()), [socket](const asio::error_code& code, size_t bytesTransferred) {
		if (!code) {
			std::cout << "Sent file list to client.\n";
		}
		});
}
void FileTransferServer::handlerFileTransferRequest(std::shared_ptr<tcp::socket> socket, const file_transfer::FileTransferRequest& request){
	std::string fileName = request.name();
	std::filesystem::path filePath = m_curPath / fileName;
	std::cout << "Getting ready to send file: " << filePath << '\n';
	sendFile(socket, filePath);
}
void FileTransferServer::handlerClientStatus(std::shared_ptr<tcp::socket> socket, const file_transfer::ClientStatus& status){
	bool success = status.success();
	success ? std::cout << "Client reported success.\n" : std::cout << "Client reported failure.\n";
	if(!success){
		// TODO: RESET
	}
}
void FileTransferServer::handlerError(std::shared_ptr<tcp::socket> socket, const file_transfer::Error& error){
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
void FileTransferServer::sendFile(std::shared_ptr<tcp::socket> socket, std::filesystem::path filePath){
	auto fileStream = std::make_shared<std::ifstream>(filePath, std::ios::binary);
	if(!(fileStream->is_open() && fileStream->good())){
		std::cout << "Failed to open file: " << filePath << '\n';
		sendError(socket, file_transfer::Error::NOT_FOUND);
		return;
	}
	fileStream->seekg(0, std::ios::beg);
	sendFileInfo(socket, filePath, [this, socket, fileStream](){ sendFileHelper(socket, fileStream, 0);});
}
void FileTransferServer::sendFileHelper(std::shared_ptr<tcp::socket> socket, std::shared_ptr<std::ifstream> fileStream, uint32_t chunkId){
	constexpr static size_t maxChunkSize = 1024 * 16; // 16KB
	if(!(fileStream->is_open() && fileStream->good())){
		std::cout << "File status not good" << '\n';
		sendError(socket, file_transfer::Error::INTERNAL);
		return;
	}
	if(fileStream->eof()){
		std::cout << "Finished sending file." << '\n';
		sendFileTransferComplete(socket);
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
	std::string serializedData = message.SerializeAsString();
	asio::async_write(*socket, asio::buffer(serializedData.data(), serializedData.size()), 	[this, socket, fileStream, chunkId](const asio::error_code& code, size_t bytesTransferred) {
		if (!code) {
			std::cout << "Sent chunk " << chunkId << " to client.\n";
			sendFileHelper(socket, fileStream, chunkId + 1);
		} else {
			std::cout << "Failed to send chunk " << chunkId << " to client. Error: " << code.message() << '\n';
			sendError(socket, file_transfer::Error::INTERNAL);
		}
		});
}

void FileTransferServer::sendError(std::shared_ptr<tcp::socket> socket, file_transfer::Error::Code code){
	file_transfer::Message message;
	message.mutable_error()->set_code(code);
	std::string serializedData = message.SerializeAsString();
	asio::async_write(*socket, asio::buffer(serializedData.data(), serializedData.size()), [code](const asio::error_code& code, size_t bytesTransferred) {
		if (!code) {
			std::cout << "Sent error to client. Code: " << code << '\n';
		}
		});
}

void FileTransferServer::sendFileTransferComplete(std::shared_ptr<tcp::socket> socket){
	file_transfer::Message message;
	message.mutable_file_transfer_complete();
	std::string serializedData = message.SerializeAsString();
	asio::async_write(*socket, asio::buffer(serializedData.data(), serializedData.size()), [](const asio::error_code& code, size_t bytesTransferred) {
		if (!code) {
			std::cout << "Sent file transfer complete to client.\n";
		}
		});
}

void FileTransferServer::sendFileInfo(std::shared_ptr<tcp::socket> socket, const std::filesystem::path& filePath, std::function<void()> onSent){
	file_transfer::Message message;
	auto* fileInfo = message.mutable_file_info();
	fileInfo->set_name(filePath.filename().string());
	fileInfo->set_size(std::filesystem::file_size(filePath));
	std::string serializedData = message.SerializeAsString();
	asio::async_write(*socket, asio::buffer(serializedData.data(), serializedData.size()), [onSent](const asio::error_code& code, size_t bytesTransferred) {
		if (!code) {
			std::cout << "Sent file info to client.\n";
			onSent();
		}
		});
}