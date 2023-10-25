#pragma once
#include <asio.hpp>
#include <fstream>
#include <array>
#include <filesystem>
#include <string_view>
#include <exception>
#include <chrono>
#include <memory>

using namespace asio::ip;

class FileTransferServer
{
private:
	asio::io_service& m_service;
	tcp::endpoint m_endpoint;
	std::filesystem::path m_curPath;
public:
	FileTransferServer(asio::io_service& service, unsigned port);
	void start(std::string_view fileName);
private:
	void acceptClient(tcp::acceptor& acceptor, std::string_view fileName);
	void handleClient(std::shared_ptr<tcp::socket> socket, std::string_view fileName);
	void writeHandler(std::shared_ptr<tcp::socket> socket, std::shared_ptr<std::ifstream> file, std::shared_ptr<std::array<char, 1024>> data);
};

