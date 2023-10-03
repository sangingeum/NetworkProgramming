#pragma once
#include <asio.hpp>
#include <fstream>
#include <array>
#include <filesystem>
#include <string_view>
#include <exception>
#include <format>
#include <functional>

using namespace asio::ip;

class FileTransferServer
{
private:
	asio::io_service& m_service;
	tcp::endpoint m_endpoint;
	tcp::socket m_socket;
	tcp::acceptor m_acceptor;
	std::ifstream m_inFile;
	std::array<char, 1024> m_data{};
	std::filesystem::path m_curPath;
public:
	FileTransferServer(asio::io_service& service, unsigned port);
	void asyncSendFile(std::string_view fileName);
private:
	void writeHandler(const asio::error_code& code, size_t bytesTransferred);
	void acceptHandler(const asio::error_code& code);
};

