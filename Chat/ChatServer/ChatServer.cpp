#include "ChatServer.hpp"
ChatServer::ChatServer(asio::io_service& service, unsigned port)
	:m_service(service), m_endpoint(tcp::v4(), port)
{}

void ChatServer::start() {
	tcp::acceptor acceptor{ m_service, m_endpoint };
	acceptClient(acceptor);
	m_service.run();
}

void ChatServer::acceptClient(tcp::acceptor& acceptor) {
	acceptor.listen();
	auto socket = std::make_shared<tcp::socket>(m_service);
	acceptor.async_accept(*socket, [this, &acceptor, socket](const asio::error_code& code) {
		if (!code) {
			appendClient(socket);
		}
		acceptClient(acceptor);
		});
}

void ChatServer::handleClient(std::shared_ptr<tcp::socket> socket) {
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

void ChatServer::readHandler(std::shared_ptr<tcp::socket> socket, std::string dataLeft, const asio::error_code& code, size_t bytesRead) {
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
				timer.async_wait([socket, this, dataLeft = std::move(dataLeft), &code, bytesRead](const asio::error_code& code) mutable {
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

void ChatServer::updateClient(ChatMessage msg) {
	auto serialization = msg.serialize();
	for (auto& socket : m_clients) {
		asio::async_write(*socket, asio::buffer(serialization.data(), serialization.size()), [this, socket](const asio::error_code& code, size_t bytesTransferred) {
			if (code) {
				removeClient(socket);
			}
			});
	}
}

void ChatServer::appendMessage(ChatMessage msg) {
	std::cout << msg.getName() << ": " << msg.getContent() << "\n";
	m_messages.push_back(msg);
	updateClient(std::move(msg));
}

void ChatServer::appendClient(std::shared_ptr<tcp::socket> socket) {
	std::cout << "A client entered the chat\n";
	m_clients.push_back(socket);
	handleClient(socket);
}

void ChatServer::removeClient(std::shared_ptr<tcp::socket> socket) {
	auto it = std::remove(m_clients.begin(), m_clients.end(), socket);
	if (it != m_clients.end()) {
		m_clients.erase(it);
		std::cout << "A client has left the chat\n";
	}
}