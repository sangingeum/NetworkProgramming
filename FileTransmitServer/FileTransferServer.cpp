#include "FileTransferServer.hpp"

FileTransferServer::FileTransferAgent::FileTransferAgent(std::filesystem::path filePath, tcp::socket&& socket)
	: m_socket{ std::move(socket) }
{
	m_inFile.open(filePath, std::ios::binary | std::ios::_Nocreate);
	if (!m_inFile)
		throw std::invalid_argument("Cannot open file");
	asio::error_code code;
	while (!code) {
		m_inFile.read(m_data.data(), m_data.size());
		size_t bytesRead = m_inFile.gcount();
		if (bytesRead > 0)
			m_socket.write_some(asio::buffer(m_data, bytesRead), code);
		else
			break;
	}
	m_socket.shutdown(tcp::socket::shutdown_send);
}

FileTransferServer::FileTransferServer(asio::io_service& service, unsigned port)
	:m_service(service), m_endpoint(tcp::v4(), port),
	m_curPath(std::filesystem::current_path())
{}

FileTransferServer::~FileTransferServer() {
	m_stop = true;
}

void FileTransferServer::start(std::string_view fileName) {
	asio::error_code code;
	auto filePath{ m_curPath };
	filePath.append(fileName);
	while (!m_stop) {
		tcp::socket socket{ m_service };
		tcp::acceptor accpetor{ m_service, m_endpoint };
		accpetor.listen();
		accpetor.accept(socket, code);
		if (!code) {
			std::thread agentThread([this, filePath, movedSocket = std::move(socket)]() mutable {
				FileTransferAgent agent{ filePath, std::move(movedSocket) };
				});
			agentThread.detach();
		}
	}
}

void FileTransferServer::stop() {
	m_stop = true;
}