#include "FileTransferClient.hpp"


FileTransferClient::FileTransferClient(asio::io_service& service)
	: m_service(service), m_socket(service), m_resolver(service), m_curPath(std::filesystem::current_path())
{}

void FileTransferClient::asyncGetFile(std::string_view fileName, std::string_view host, std::string_view port) {
	tcp::resolver::query query{ host.data() ,port.data() };
	m_resolver.async_resolve(query, [this, fileName](const asio::error_code& code, tcp::resolver::iterator it) {
		resolverHandler(fileName, code, it);
		});
}

void FileTransferClient::readHandler(const asio::error_code& code, size_t bytesTransferred) {
	if (!code) {
		m_outFile.write(m_data.data(), bytesTransferred);
		m_socket.async_read_some(asio::buffer(m_data, m_data.size()), std::bind(&FileTransferClient::readHandler, this, std::placeholders::_1, std::placeholders::_2));
	}
}

void FileTransferClient::connectHandler(std::string_view fileName, const asio::error_code& code) {
	if (!code) {
		auto filePath = m_curPath;
		filePath.append(fileName);
		m_outFile.open(filePath, std::ios::binary | std::ios::trunc);
		if (!m_outFile)
			throw std::invalid_argument(std::format("Cannot open file {}\n", fileName));
		m_socket.async_read_some(asio::buffer(m_data, m_data.size()), std::bind(&FileTransferClient::readHandler, this, std::placeholders::_1, std::placeholders::_2));
	}
}

void FileTransferClient::resolverHandler(std::string_view fileName, const asio::error_code& code, tcp::resolver::iterator it) {
	if (!code) {
		m_socket.async_connect(*it, [this, fileName](const asio::error_code& code) {
			connectHandler(fileName, code);
			});
	}
}