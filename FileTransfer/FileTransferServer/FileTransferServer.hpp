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
	void handleRead(std::shared_ptr<tcp::socket> socket, std::shared_ptr<std::array<std::byte, 1024>> data, const asio::error_code& code, size_t bytesTransferred);
	// Message handlers
	void handleFileListRequest(std::shared_ptr<tcp::socket> socket);
	void handlerFileTransferRequest(std::shared_ptr<tcp::socket> socket, const file_transfer::FileTransferRequest& request);
	void handlerClientStatus(std::shared_ptr<tcp::socket> socket, const file_transfer::ClientStatus& status);
	void handlerError(std::shared_ptr<tcp::socket> socket, const file_transfer::Error& error);
	// Send
	void sendFile(std::shared_ptr<tcp::socket> socket, std::filesystem::path filePath);
	void sendFileHelper(std::shared_ptr<tcp::socket> socket, std::shared_ptr<std::ifstream> fileStream, uint32_t chunkId);
	void sendError(std::shared_ptr<tcp::socket> socket, file_transfer::Error::Code code);
	void sendFileTransferComplete(std::shared_ptr<tcp::socket> socket);
	void sendFileInfo(std::shared_ptr<tcp::socket> socket, const std::filesystem::path& filePath, std::function<void()> onSent = [](){});
};
