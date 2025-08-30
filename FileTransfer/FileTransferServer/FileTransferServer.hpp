#pragma once
#include <asio.hpp>
#include <fstream>
#include <array>
#include <filesystem>
#include <string_view>
#include <exception>
#include <chrono>
#include <memory>
#include "FileTransfer.pb.h"

using namespace asio::ip;

class FileTransferServer
{
private:
	asio::io_context& m_context;
	tcp::endpoint m_endpoint;
	std::filesystem::path m_curPath;
public:
	FileTransferServer(asio::io_context& service, unsigned port);
	void start();
private:
	void acceptClient(std::shared_ptr<tcp::acceptor> acceptor);
	void handleClient(std::shared_ptr<tcp::socket> socket);
	void writeHandler(std::shared_ptr<tcp::socket> socket, std::shared_ptr<std::array<char, 1024>> data);
	void readHandler(std::shared_ptr<tcp::socket> socket, std::shared_ptr<std::array<std::byte, 1024>> data, const asio::error_code& code, size_t bytesTransferred);
};

