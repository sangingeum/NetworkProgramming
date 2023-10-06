#pragma once
#include <asio.hpp>
#include <string>
#include <string_view>
#include "Chat.pb.h"
using namespace asio::ip;

class ChatClient
{

private:
	asio::io_service& m_service;
	tcp::socket m_socket;
	std::string_view m_name;
	Chat m_baseChat;
public:

	ChatClient(asio::io_service& service, std::string_view name)
		: m_service(service), m_socket(service), m_name(name)
	{
		m_baseChat.set_name(name.data());
	}

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
		Chat chat{ m_baseChat };
		chat.set_content(content.data());
		asio::error_code code;
		// 
		m_socket.write_some(chat.SerializeAsString(), code);
		if (code)
			return false;
		return true;
	}

	void close() {
		if (m_socket.is_open()) {
			m_socket.shutdown(tcp::socket::shutdown_both);
			m_socket.close();
		}
	}

	void changeName(std::string_view name) {
		m_baseChat.set_name(name.data());
	}
private:


};

