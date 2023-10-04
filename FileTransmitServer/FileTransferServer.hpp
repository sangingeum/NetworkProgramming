#pragma once
#include <asio.hpp>
#include <fstream>
#include <array>
#include <filesystem>
#include <string_view>
#include <exception>
#include <chrono>

using namespace std::chrono_literals;
using namespace asio::ip;

class FileTransferServer
{
private:
	class FileTransferAgent {
	private:
		asio::io_service& m_service;
		tcp::socket m_socket;
		std::ifstream m_inFile;
		std::array<char, 1024> m_data{};
	public:
		FileTransferAgent(asio::io_service& service, std::filesystem::path filePath, tcp::socket&& socket);
	};

	asio::io_service& m_service;
	tcp::endpoint m_endpoint;
	std::filesystem::path m_curPath;
	std::atomic<bool> m_stop{ false };
	std::vector<FileTransferAgent> agents;
public:
	FileTransferServer(asio::io_service& service, unsigned port);
	~FileTransferServer();
	void start(std::string_view fileName);
	void stop();
};

