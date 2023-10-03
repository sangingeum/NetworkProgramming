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
	FileTransferServer(asio::io_service& service, unsigned port)
		:m_service(service), m_endpoint(tcp::v4(), port), m_socket(service), m_acceptor(service, m_endpoint),
		m_curPath(std::filesystem::current_path())
	{}

	void asyncSendFile(std::string_view fileName) {
		auto filePath = m_curPath;
		filePath.append(fileName);
		m_inFile.open(filePath, std::ios::binary | std::ios::_Nocreate);
		if(!m_inFile)
			throw std::invalid_argument(std::format("Cannot open file {}\n", fileName));
		m_acceptor.listen();
		m_acceptor.async_accept(m_socket, std::bind(&FileTransferServer::acceptHandler, this, std::placeholders::_1));
	}

private:

	void writeHandler(const asio::error_code& code, size_t bytesTransferred) {
		if (!code) {
			if (m_inFile) {
				m_inFile.read(m_data.data(), m_data.size());
				size_t bytesRead = m_inFile.gcount();
				if (bytesRead > 0)
					m_socket.async_write_some(asio::buffer(m_data, bytesRead), std::bind(&FileTransferServer::writeHandler, this, std::placeholders::_1, std::placeholders::_2));
				else
					m_socket.shutdown(tcp::socket::shutdown_send);
			}
		}
		else {
			m_socket.shutdown(tcp::socket::shutdown_send);
		}
	}

	void acceptHandler(const asio::error_code& code) {
		if (!code) {
			if (m_inFile) {
				m_inFile.read(m_data.data(), m_data.size());
				size_t bytesRead = m_inFile.gcount();
				if (bytesRead > 0)
					m_socket.async_write_some(asio::buffer(m_data, bytesRead), std::bind(&FileTransferServer::writeHandler, this, std::placeholders::_1, std::placeholders::_2));
				else
					m_socket.shutdown(tcp::socket::shutdown_send);
			}
			else {
				m_socket.shutdown(tcp::socket::shutdown_send);
			}
		}
	}
};

