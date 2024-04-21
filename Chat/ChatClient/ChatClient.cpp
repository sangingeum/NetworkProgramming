#include "ChatClient.hpp"

ChatClient::ChatClient(asio::io_service& service, std::string_view name)
	: m_service(service), m_socket(service), m_name(name)
{}
ChatClient::~ChatClient() {
	close();
}
void ChatClient::readHandler(std::string dataLeft)
{
	auto dataPtr = std::make_shared<std::array<char, ChatMessage::maxLength>>();
	auto& data = *dataPtr;
	m_socket.async_read_some(asio::buffer(data.data(), data.size()), [dataPtr, this, dataLeft](const asio::error_code& code, size_t bytesRead) {
		auto& data = *dataPtr;
		if (!code) {
			std::string dataString = dataLeft + std::string{ data.data(), bytesRead };
			ChatMessage msg;
			if (msg.deSerialize(dataString)) {
				std::cout << msg.getName() << ": " << msg.getContent() << "\n";
			}
			if (!m_stopReading.load())
				readHandler(std::move(dataString));
		}
		else {
			std::cout << "The server is not responding...\nTerminating the chat...\n";
			close();
		}
		});
}
bool ChatClient::connect(std::string_view address, std::string_view port) {
	if (m_socket.is_open())
		m_socket.close();
	tcp::resolver resolver{ m_service };
	tcp::resolver::query query{ address.data(), port.data() };
	asio::error_code code;
	auto it = resolver.resolve(query, code);
	if (code)
		return false;
	m_socket.connect(*it, code);
	if (code)
		return false;
	return true;
}

bool ChatClient::send(std::string_view content) {
	if (!m_socket.is_open())
		return false;
	ChatMessage chat{ m_name, content };
	if (chat.getSize() >= ChatMessage::maxLength)
		return false;
	asio::error_code code;
	auto serialization = chat.serialize();
	try {
		asio::write(m_socket, asio::buffer(serialization, serialization.size()));
	}
	catch (std::exception& e) {
		return false;
	}
	return true;
}

std::vector<ChatMessage> ChatClient::read() {
	if (!m_socket.is_open())
		return {};
	if (m_socket.available() <= 0)
		return {};
	std::array<char, ChatMessage::maxLength> data;
	asio::error_code code;
	std::string curString = m_leftString;
	while (!code && m_socket.available() > 0) {
		uint32_t bytesRead = m_socket.read_some(asio::buffer(data.data(), data.size()), code);
		if (bytesRead > 0) {
			curString.append(data.data(), bytesRead);
		}
		else
			break;
	}
	ChatMessage msg;
	std::vector<ChatMessage> messages;
	while (msg.deSerialize(curString)) {
		messages.push_back(msg);
	}
	m_leftString = curString;
	return messages;
}

void ChatClient::start()
{
	std::jthread reader{ [this]() { readHandler(); m_service.run(); } };
	while (true) {
		// Read user input
		std::string content;
		std::getline(std::cin, content, '\n');
		// Clear the previous line
		std::cout << "\033[F\r" << std::string(content.size() + 1, ' ') << "\r";
		if (!this->send(content))
			std::cout << "Cannot send the message. The server is down or your message is too long.\n";
		if (m_stopReading.load())
			break;
	}
}

void ChatClient::close() {
	if (m_socket.is_open()) {
		m_socket.shutdown(tcp::socket::shutdown_both);
		m_socket.close();
		m_stopReading.store(true);
	}
}

void ChatClient::setName(std::string_view name) {
	m_name = name;
}