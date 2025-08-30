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
	std::cout << "Handling client...\n";
	std::shared_ptr<std::array<std::byte, 1024>> data = std::make_shared<std::array<std::byte, 1024>>();
	socket->async_read_some(asio::buffer(data->data(), data->size()), std::bind(&FileTransferServer::readHandler, this, socket, data, std::placeholders::_1, std::placeholders::_2));
}

void FileTransferServer::writeHandler(std::shared_ptr<tcp::socket> socket, std::shared_ptr<std::array<char, 1024>> data) {
	socket->async_write_some(asio::buffer(data->data(), data->size()), [this, socket, data](const asio::error_code& code, size_t bytesTransferred) {
		if (!code) {
			std::cout << "Sent data to client.\n";
		}
	});
}	

void FileTransferServer::readHandler(std::shared_ptr<tcp::socket> socket, std::shared_ptr<std::array<std::byte, 1024>> data, const asio::error_code& code, size_t bytesTransferred){
	std::cout << "Handling read...\n";
	if (!code) {
		std::cout << "Received data from the client.\n";
		std::cout << "bytesTransferred: " << bytesTransferred << '\n';	
		file_transfer::Message message;
		message.ParseFromArray(data->data(), static_cast<int>(bytesTransferred));
		switch (message.content_case()) {
		case file_transfer::Message::kFileListRequest:
		{
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
			socket->async_write_some(asio::buffer(serializedData.data(), serializedData.size()), [socket](const asio::error_code& code, size_t bytesTransferred) {
				if (!code) {
					std::cout << "Sent file list to client.\n";
				}
				});
			break;
		}
		case file_transfer::Message::kFileTransferRequest:
		{
			break;
		}
		case file_transfer::Message::kStatus:
		{
			break;
		}
		case file_transfer::Message::kError:
		{
			break;
		}
		default:
			std::cout << "Unknown message type received from client.\n";
			break;
		}
		socket->async_read_some(asio::buffer(data->data(), data->size()), std::bind(&FileTransferServer::readHandler, this, socket, data, std::placeholders::_1, std::placeholders::_2));
	}
}
