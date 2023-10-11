#pragma once
#include <asio.hpp>
#include <string>
#include <string_view>
#include "ChatMessage.hpp"
#include <array>
using namespace asio::ip;

class ChatClient
{
private:
	asio::io_service& m_service;
	tcp::socket m_socket;
	std::string_view m_name;
	std::string m_leftString{};
public:
	ChatClient(asio::io_service& service, std::string_view name);
	~ChatClient();
	bool connect(std::string_view address, std::string_view port);
	bool send(std::string_view content);
	std::vector<ChatMessage> read();
	void close();
	void setName(std::string_view name);
};

