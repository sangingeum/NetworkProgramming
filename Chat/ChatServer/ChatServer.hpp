#pragma once
#include <asio.hpp>
#include "ChatMessage.hpp"
#include <array>
#include <memory>
#include <vector>
#include <iostream>
#include <chrono>
#include <deque>

using namespace asio::ip;

class ChatServer
{
private:
	asio::io_service& m_service;
	tcp::endpoint m_endpoint;
	std::atomic<bool> m_stop{ false };
	std::vector<std::shared_ptr<tcp::socket>> m_clients;
	std::vector<ChatMessage> m_messages;
	std::deque<asio::steady_timer> m_timers;
	const std::chrono::seconds m_recheckTime{ 1 };
public:
	ChatServer(asio::io_service& service, unsigned port)
		:m_service(service), m_endpoint(tcp::v4(), port)
	{}
	~ChatServer() {
		stop();
	}
	void start() {
		m_stop = false;
		tcp::acceptor acceptor{ m_service, m_endpoint };
		acceptClient(acceptor);
		m_service.run();
	}
	void stop() {
		m_stop = true;
	}
private:

	void acceptClient(tcp::acceptor& acceptor) {
		acceptor.listen();
		auto socket = std::make_shared<tcp::socket>(m_service);
		acceptor.async_accept(*socket, [this, &acceptor, socket](const asio::error_code& code) {
			if (!code) {
				m_clients.push_back(socket);
				handleClient(socket);
			}
			acceptClient(acceptor);
			});
	}

	void handleClient(std::shared_ptr<tcp::socket> socket) {
		// Handle Read
		std::array<char, 1024> data;
		socket->async_read_some(asio::buffer(data.data(), data.size()), [socket, &data, this](const asio::error_code& code, size_t bytesRead) {
			if (!code) {
				std::string dataString{ data.data(), bytesRead };
				ChatMessage msg;
				if (msg.deSerialize(dataString)) {
					appendMessage(std::move(msg));
				}
				readHandler(socket, std::move(dataString), code, bytesRead);
			}
			});
	}

	void readHandler(std::shared_ptr<tcp::socket> socket, std::string dataLeft, const asio::error_code& code, size_t bytesRead) {
		std::array<char, 1024> data;
		socket->async_read_some(asio::buffer(data.data(), data.size()), [socket, &data, this, &dataLeft](const asio::error_code& code, size_t bytesRead) {
			if (!code) {
				dataLeft.append(data.data(), bytesRead);
				ChatMessage msg;
				if (msg.deSerialize(dataLeft)) {
					appendMessage(std::move(msg));
				}
				readHandler(socket, std::move(dataLeft), code, bytesRead);
			}
			else {
				// Remove from the list?
				if (code.value() == 2) // End of file error
				{
					m_timers.push_back(asio::steady_timer{ m_service });
					auto& timer = m_timers.back();
					timer.expires_after(m_recheckTime);
					timer.async_wait([socket, &data, this, &dataLeft, &code, bytesRead](const asio::error_code& code) {
						readHandler(socket, std::move(dataLeft), code, bytesRead);
						if (m_timers.size() > 1)
							m_timers.pop_front();
						});
				}
				else
					removeClient(socket);
			}
			});
	}

	void writeHandler(std::shared_ptr<tcp::socket> socket, size_t bytesRead) {

	}

	void appendMessage(ChatMessage msg) {
		std::cout << msg.getName() << ": " << msg.getContent() << "\n";
		m_messages.push_back(msg);
		updateClient(std::move(msg));
	}

	void updateClient(ChatMessage msg) {
		/*
		for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
			auto& socket = (**it);
			if (socket.is_open()) {
				socket.async_send(std::bu)
			}
		}
		*/
	}


	void removeClient(std::shared_ptr<tcp::socket> socket) {
		auto it = std::remove(m_clients.begin(), m_clients.end(), socket);
		if (it != m_clients.end())
			m_clients.erase(it);
	}

};
