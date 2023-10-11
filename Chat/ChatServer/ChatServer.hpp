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
	ChatServer(asio::io_service& service, unsigned port);
	void start();
private:
	void acceptClient(tcp::acceptor& acceptor);
	void handleClient(std::shared_ptr<tcp::socket> socket);
	void readHandler(std::shared_ptr<tcp::socket> socket, std::string dataLeft, const asio::error_code& code, size_t bytesRead);
	void updateClient(ChatMessage msg);
	void appendMessage(ChatMessage msg);
	void appendClient(std::shared_ptr<tcp::socket> socket);
	void removeClient(std::shared_ptr<tcp::socket> socket);
};
