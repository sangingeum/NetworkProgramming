#pragma once
#include "ClientSession.hpp"

class FileTransferClient
{
private:
	asio::io_context& m_context;
public:
	FileTransferClient(asio::io_context& service);
	void connect(std::string_view host, std::string_view port);
private:
	void handleResolve(const asio::error_code& code, tcp::resolver::iterator it);
	void handleConnectionSuccess(std::shared_ptr<tcp::socket> socket, const asio::error_code& code);
	void handleConnectionFailure(std::shared_ptr<tcp::socket> socket, tcp::resolver::iterator it, const asio::error_code& code);
};

