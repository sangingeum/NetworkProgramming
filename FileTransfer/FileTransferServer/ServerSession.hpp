#pragma once
#include <memory>
#include <asio.hpp>
#include "FileTransfer.pb.h"
#include "Parser.hpp"
#include "Packager.hpp"
#include <filesystem>
#include <fstream>
#include <array>
#include <string_view>
#include <exception>
#include <chrono>
#include <map>

using namespace asio::ip;

class ServerSession : public std::enable_shared_from_this<ServerSession>
{
private:
    std::shared_ptr<tcp::socket> m_socket;
    Parser m_parser;
    std::filesystem::path m_curPath;
    uint32_t m_identifierCounter;
    std::map<uint32_t, std::shared_ptr<std::ifstream>> m_activeFileTransfers;
public:
    ServerSession(std::shared_ptr<tcp::socket> socket);
    void start();
    // Read
    void handleRead(std::shared_ptr<ReadBuffer> data, const asio::error_code& code, size_t bytesTransferred);
private:
    // Message handlers
    void handleFileListRequest(const file_transfer::FileListRequest& request);
    void handleFileTransferRequest(const file_transfer::FileTransferRequest& request);
    void handleClientReady(const file_transfer::ClientReady& ready);
    void handleError(const file_transfer::Error& error);
    void handleFileTransferError(const file_transfer::FileTransferError& error);
    // Send
    void sendFileHelper(uint32_t fileIdentifier, uint32_t chunkId);
    void sendFile(uint32_t fileIdentifier);
    void sendError(file_transfer::ErrorCode code);
    void sendFileTransferError(file_transfer::ErrorCode code, uint32_t fileIdentifier, std::string_view errorMessage);
    void sendFileTransferComplete(uint32_t fileIdentifier);
    void sendFileInfo(const std::filesystem::path& filePath, uint32_t fileIdentifier);
    // Utilities
    inline uint32_t getNextIdentifier(){
        return m_identifierCounter++;
    }
};