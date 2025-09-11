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

using namespace asio::ip;

class ServerSession : public std::enable_shared_from_this<ServerSession>
{
private:
    std::shared_ptr<tcp::socket> m_socket;
    Parser m_parser;
    std::filesystem::path m_curPath;
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
    void handleClientAcknowledgement(const file_transfer::ClientAcknowledgement& ack);
    void handleError(const file_transfer::Error& error);
    // Send
    void sendFile(std::filesystem::path filePath);
    void sendFileHelper(std::shared_ptr<std::ifstream> fileStream, uint32_t chunkId);
    void sendError(file_transfer::Error::Code code);
    void sendFileTransferComplete();
    void sendFileInfo(const std::filesystem::path& filePath, std::function<void()> onSent = []() {});
};