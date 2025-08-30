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

class FileTransferClient
{
private:
	tcp::socket m_socket;
	tcp::resolver m_resolver;
	std::ofstream m_outFile;
	std::array<char, 1024> m_data{};
	std::filesystem::path m_curPath;
public:
	FileTransferClient(asio::io_context& service);
	void asyncGetFile(std::string_view fileName, std::string_view host, std::string_view port);
private:
	void readHandler(const asio::error_code& code, size_t bytesTransferred);
	void connectHandler(std::string_view fileName, const asio::error_code& code);
	void resolverHandler(std::string_view fileName, const asio::error_code& code, tcp::resolver::iterator it);

};

