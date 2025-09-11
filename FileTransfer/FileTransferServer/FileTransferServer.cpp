#include "FileTransferServer.hpp"
#include <iostream>

FileTransferServer::FileTransferServer(asio::io_context& context, unsigned port)
	: m_context(context), m_endpoint(tcp::v4(), port)
{
	std::cout << "Server starting on port " << port << '\n';
}

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
	auto session = std::make_shared<ServerSession>(socket);
	session->start();
}

