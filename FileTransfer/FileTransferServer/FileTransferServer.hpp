#pragma once
#include "ServerSession.hpp"

class FileTransferServer
{
private:
	asio::io_context& m_context;
	tcp::endpoint m_endpoint;
public:
	FileTransferServer(asio::io_context& service, unsigned port);
	void start();
private:
	void acceptClient(std::shared_ptr<tcp::acceptor> acceptor);
	void handleClient(std::shared_ptr<tcp::socket> socket);
};
