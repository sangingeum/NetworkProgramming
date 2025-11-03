#pragma once

#include <asio.hpp>
#include <array>
#include <string_view>
#include <fstream>
#include <filesystem>
#include "FileTransfer.pb.h"
#include "Parser.hpp"
#include "Packager.hpp"
#include <map>

using namespace asio::ip;

class ClientSession : public std::enable_shared_from_this<ClientSession>
{
private:
    std::shared_ptr<tcp::socket> m_socket;
    Parser m_parser;
    std::filesystem::path m_curPath;
public:
    ClientSession(std::shared_ptr<tcp::socket> socket);
    void start();
    void sendFileTransferRequest(std::string_view fileName);
	void sendFileListRequest();
	void sendReady();
private:
    void handleRead(std::shared_ptr<ReadBuffer> buffer, const asio::error_code& code, size_t bytesTransferred);
	// Message handlers
	void handleFileList(const file_transfer::FileList& list);
	void handleFileInfo(const file_transfer::FileInfo& info);
	void handleFileChunk(const file_transfer::FileChunk& chunk);
	void handleFileTransferCompletion(const file_transfer::FileTransferComplete& complete);
	void handleError(const file_transfer::Error& error);
	// File operations
	std::string generateRandomUniqueFilename();
};

