#pragma once
#include <asio.hpp>
#include <array>
#include <string_view>
#include <fstream>
#include <filesystem>
#include "FileTransfer.pb.h"
#include <map>
using namespace asio::ip;

class FileTransferClient
{
private:
	constexpr static size_t bufferSize = 1024*16;
	using ReadBuffer = std::array<char, bufferSize>;
	asio::io_context& m_context;
	std::map<std::string, std::ofstream> m_outFiles;
	std::ofstream m_outFile;
	std::filesystem::path m_curPath;
public:
	FileTransferClient(asio::io_context& service);
	void connect(std::string_view host, std::string_view port);
	void sendFileTransferRequest(std::shared_ptr<tcp::socket> socket, std::string_view fileName);
	void sendFileListRequest(std::shared_ptr<tcp::socket> socket);
	void sendAcknowledgement(std::shared_ptr<tcp::socket> socket, bool success);
	void sendReady(std::shared_ptr<tcp::socket> socket);
private:
	void handleRead(std::shared_ptr<tcp::socket> socket, std::shared_ptr<ReadBuffer> buffer, const asio::error_code& code, size_t bytesTransferred);
	void handleConnectionSuccess(std::shared_ptr<tcp::socket> socket, const asio::error_code& code);
	void handleConnectionFailure(std::shared_ptr<tcp::socket> socket, tcp::resolver::iterator it, const asio::error_code& code);
	void handleResolve(const asio::error_code& code, tcp::resolver::iterator it);
	// Message handlers
	void handleFileList(const file_transfer::FileList& list);
	void handleFileInfo(const file_transfer::FileInfo& info);
	void handleFileChunk(const file_transfer::FileChunk& chunk);
	void handleFileTransferCompletion(const file_transfer::FileTransferComplete& complete);
	void handleError(const file_transfer::Error& error);
	// File operations
	std::string generateRandomUniqueFilename();
};

