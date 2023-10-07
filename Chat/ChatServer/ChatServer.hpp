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
	std::vector<std::shared_ptr<tcp::socket>> m_clients;
	std::vector<ChatMessage> m_messages;
	std::deque<asio::steady_timer> m_timers;
	const std::chrono::seconds m_recheckTime{ 1 };
	static constexpr uint32_t BufferSize{ 2048 };
public:
	ChatServer(asio::io_service& service, unsigned port)
		:m_service(service), m_endpoint(tcp::v4(), port)
	{}
	void start() {
		tcp::acceptor acceptor{ m_service, m_endpoint };
		acceptClient(acceptor);
		m_service.run();
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
		auto dataPtr = std::make_shared<std::array<char, BufferSize>>();
		auto& data = *dataPtr;
		socket->async_read_some(asio::buffer(data.data(), data.size()), [socket, dataPtr, this](const asio::error_code& code, size_t bytesRead) {
			auto& data = *dataPtr;
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
		auto dataPtr = std::make_shared<std::array<char, BufferSize>>();
		auto& data = *dataPtr;
		socket->async_read_some(asio::buffer(data.data(), data.size()), [socket, dataPtr, this, dataLeft](const asio::error_code& code, size_t bytesRead) mutable {
			auto& data = *dataPtr;
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
					// Call the readHandler after a while
					m_timers.push_back(asio::steady_timer{ m_service });
					auto& timer = m_timers.back();
					timer.expires_after(m_recheckTime);
					timer.async_wait([socket, &data, this, dataLeft, &code, bytesRead](const asio::error_code& code) mutable {
						readHandler(socket, std::move(dataLeft), code, bytesRead);
						if (m_timers.size() > 1)
							m_timers.pop_front();
						});
				}
				else {
					removeClient(socket);
				}
			}
			});
	}

	void updateClient(ChatMessage msg) {
		auto serialization = msg.serialize();
		for (auto& socket : m_clients) {
			asio::async_write(*socket, asio::buffer(serialization.data(), serialization.size()), [this, socket](const asio::error_code& code, size_t bytesRead) {
				if (code) {
					removeClient(socket);
				}
				});
		}
	}

	void appendMessage(ChatMessage msg) {
		std::cout << msg.getName() << ": " << msg.getContent() << "\n";
		m_messages.push_back(msg);
		updateClient(std::move(msg));
	}

	void removeClient(std::shared_ptr<tcp::socket> socket) {
		auto it = std::remove(m_clients.begin(), m_clients.end(), socket);
		if (it != m_clients.end()) {
			m_clients.erase(it);
			std::cout << "A client left\n";
		}

	}

};
