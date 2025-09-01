#pragma once
#include <asio.hpp>
#include <array>
#include <string_view>
#include <fstream>
#include <filesystem>
#include "FileTransfer.pb.h"

using namespace asio::ip;

class FileTransferClient
{
private:
	constexpr static size_t bufferSize = 1024*16;
	using ReadBuffer = std::array<char, bufferSize>;
	asio::io_context& m_context;
	std::ofstream m_outFile;
	std::filesystem::path m_curPath;
public:
	FileTransferClient(asio::io_context& service);
	void connect(std::string_view host, std::string_view port);
	void sendFileTransferRequest(std::shared_ptr<tcp::socket> socket, std::string_view fileName);
	void sendFileListRequest(std::shared_ptr<tcp::socket> socket);
	void sendStatus(std::shared_ptr<tcp::socket> socket, bool success);
private:
	void readHandler(std::shared_ptr<tcp::socket> socket, std::shared_ptr<ReadBuffer> buffer, const asio::error_code& code, size_t bytesTransferred);
	void connectionSuccessHandler(std::shared_ptr<tcp::socket> socket, const asio::error_code& code);
	void connectionFailureHandler(std::shared_ptr<tcp::socket> socket, tcp::resolver::iterator it, const asio::error_code& code);
	void resolverHandler(const asio::error_code& code, tcp::resolver::iterator it);
	// Message handlers
	void fileListHandler(const file_transfer::FileList& list);
	void fileInfoHandler(const file_transfer::FileInfo& info);
	void fileChunkHandler(const file_transfer::FileChunk& chunk);
	void fileTransferCompleteHandler(const file_transfer::FileTransferComplete& complete);
	void erorrHandler(const file_transfer::Error& error);
};

