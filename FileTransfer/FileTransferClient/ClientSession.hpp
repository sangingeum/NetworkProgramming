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
#include <iostream>
#include <memory>

using namespace asio::ip;

class ClientSession : public std::enable_shared_from_this<ClientSession>
{
private:
    std::shared_ptr<tcp::socket> m_socket;
    Parser m_parser;
    std::filesystem::path m_curPath;
	std::map<uint32_t, std::shared_ptr<std::ofstream>> m_activeFileTransfers; // map of file ID to file stream
	std::map<uint32_t, std::string> m_activeOriginalFileNames; // map of file ID to original file name
	std::map<uint32_t, std::string> m_activeTempFileNames; // map of file ID to temp file name
public:
    ClientSession(std::shared_ptr<tcp::socket> socket);
    void start();
    void sendFileTransferRequest(std::string_view fileName);
	void sendFileListRequest();
	void sendReady(uint32_t fileIdentifier);
private:
	// Utilities
	void resetFileTransferState(uint32_t fileIdentifier);
	// Send
	void sendFileTransferError(file_transfer::ErrorCode code, uint32_t fileIdentifier, std::string_view errorMessage);
	// Message handlers
	void handleRead(std::shared_ptr<ReadBuffer> buffer, const asio::error_code& code, size_t bytesTransferred);
	void handleFileList(const file_transfer::FileList& list);
	void handleFileInfo(const file_transfer::FileInfo& info);
	void handleFileChunk(const file_transfer::FileChunk& chunk);
	void handleFileTransferComplete(const file_transfer::FileTransferComplete& complete);
	void handleError(const file_transfer::Error& error);
	void handleFileTransferError(const file_transfer::FileTransferError& error);
	// File operations
	std::string generateRandomUniqueFilename();
	
};

