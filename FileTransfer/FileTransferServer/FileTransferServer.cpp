#include "FileTransferServer.hpp"
FileTransferServer::FileTransferServer(asio::io_context& context, unsigned port)
	: m_context(context), m_endpoint(tcp::v4(), port),
	m_curPath(std::filesystem::current_path())
{}

void FileTransferServer::start(std::string_view fileName) {
	tcp::acceptor acceptor{ m_context, m_endpoint };
	acceptClient(acceptor, fileName);
	m_context.run();
}

void FileTransferServer::acceptClient(tcp::acceptor& acceptor, std::string_view fileName) {
	auto socket = std::make_shared<tcp::socket>(m_context);
	acceptor.async_accept(*socket, [this, &acceptor, socket, fileName](const asio::error_code& code) {
		if (!code) {
			handleClient(socket, fileName);
		}
		acceptClient(acceptor, fileName);
		});
}

void FileTransferServer::handleClient(std::shared_ptr<tcp::socket> socket, std::string_view fileName) {
	auto filePath{ m_curPath };
	filePath.append(fileName);
	auto file = std::make_shared<std::ifstream>();
	auto data = std::make_shared<std::array<char, 1024>>();
	file->open(filePath, std::ios::binary | std::ios::_Nocreate);
	if (!file) {
		// Cannot open file
		throw std::invalid_argument("Cannot open file");
	}
	file->read(data->data(), data->size());
	size_t bytesRead = file->gcount();
	socket->async_write_some(asio::buffer(data->data(), bytesRead), [this, socket, file, data](const asio::error_code& code, size_t bytesTransferred) {
		if (bytesTransferred > 0 && !code) {
			writeHandler(socket, file, data);
		}
		});
}

void FileTransferServer::writeHandler(std::shared_ptr<tcp::socket> socket, std::shared_ptr<std::ifstream> file, std::shared_ptr<std::array<char, 1024>> data) {
	file->read(data->data(), data->size());
	size_t bytesRead = file->gcount();
	socket->async_write_some(asio::buffer(data->data(), bytesRead), [this, socket, file, data](const asio::error_code& code, size_t bytesTransferred) {
		if (bytesTransferred > 0 && !code) {
			writeHandler(socket, file, data);
		}
		});
}