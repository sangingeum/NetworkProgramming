#include "FileTransferClient.hpp"
#include <exception>
#include <format>
#include <functional>
#include <random>

FileTransferClient::FileTransferClient(asio::io_context& context)
	: m_context(context)
{}

void FileTransferClient::connect(std::string_view host, std::string_view port){
	// If m_session is still valid, do not attempt to connect again.
    if(!m_session.expired()){
		std::cout << "Client is already connected to a server.\n";
		return;
	}

	tcp::resolver::query query{ host.data() ,port.data() };
	auto resolver = std::make_shared<tcp::resolver>(m_context); 
	resolver->async_resolve(query, [this, resolver](const asio::error_code& code, tcp::resolver::iterator it) {
		handleResolve(code, it);
	});
}

std::shared_ptr<ClientSession> FileTransferClient::getSession(){
	if(m_session.expired()){
		return nullptr;
	}
	return m_session.lock();
}

void FileTransferClient::handleConnectionSuccess(std::shared_ptr<tcp::socket> socket, const asio::error_code& code) {
	std::cout << "Succecssfully connected to the server.\n";
	auto session = std::make_shared<ClientSession>(socket);
	m_session = session;
	session->start();
}

void FileTransferClient::handleConnectionFailure(std::shared_ptr<tcp::socket> socket, tcp::resolver::iterator it, const asio::error_code& code){
	std::cerr << "Failed to connect to the server: " << code.message() << '\n';
	socket->async_connect(*it, [this, it, socket](const asio::error_code& code) {
		if(!code){
			handleConnectionSuccess(socket, code);
		}
		else{
			handleConnectionFailure(socket, it, code);
		}
	});
}

void FileTransferClient::handleResolve(const asio::error_code& code, tcp::resolver::iterator it) {
	if (!code) {
		std::cout << "Succecssfully resolved the server address.\n";
		auto socket = std::make_shared<tcp::socket>(m_context);
		socket->async_connect(*it, [this, it, socket](const asio::error_code& code) {
			if(!code){
				handleConnectionSuccess(socket, code);
			}
			else{
				handleConnectionFailure(socket, it, code);
			}
		});
	}
}

