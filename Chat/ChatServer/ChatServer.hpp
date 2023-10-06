#pragma once
#include <asio.hpp>
#include "ChatMessage.hpp"
#include <array>
#include <iostream>
using namespace asio::ip;

class ChatServer
{
private:
	asio::io_service& m_service;
	tcp::endpoint m_endpoint;
	std::atomic<bool> m_stop{ false };

public:
	ChatServer(asio::io_service& service, unsigned port)
		:m_service(service), m_endpoint(tcp::v4(), port)
	{}
	~ChatServer() {
		stop();
	}
	void start() {
		m_stop = true;

		tcp::socket socket{ m_service };
		tcp::acceptor acceptor{ m_service, m_endpoint };
		std::array<char, 1024> data;
		acceptor.listen();
		asio::error_code code;
		acceptor.accept(socket, code);
		if (!code) {
			size_t bytesRead = socket.read_some(asio::buffer(data, data.size()), code);
			if (!code) {
				std::cout.write(data.data(), bytesRead);
				std::string received{ data.data(), bytesRead };
				ChatMessage msg;
				if (msg.deSerialize(received)) {
					std::cout << "after :" << received << "\n";
					std::cout << "name: " << msg.getName() << ", content: " << msg.getContent() << "\n";
				}
			}
		}
	}
	void stop() {
		m_stop = true;
	}
};
