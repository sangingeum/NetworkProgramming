#pragma once
#include <asio.hpp>
#include <string>
#include <string_view>
#include "ChatMessage.hpp"
using namespace asio::ip;

// chat wrapper에서 길이 해더 정하자
// https://protobuf.dev/programming-guides/techniques/
class ChatClient
{

private:
	asio::io_service& m_service;
	tcp::socket m_socket;
	std::string_view m_name;
public:

	ChatClient(asio::io_service& service, std::string_view name)
		: m_service(service), m_socket(service), m_name(name)
	{}

	bool connect(std::string_view address, std::string_view port) {
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

	bool send(std::string_view content) {
		if (!m_socket.is_open())
			return false;
		ChatMessage chat{ m_name, content };
		asio::error_code code;
		auto serialization = chat.serialize();
		asio::write(m_socket, asio::buffer(serialization, serialization.size()));
		return true;
	}

	void close() {
		if (m_socket.is_open()) {
			m_socket.shutdown(tcp::socket::shutdown_both);
			m_socket.close();
		}
	}

	void changeName(std::string_view name) {
		m_name = name;
	}
private:


};

