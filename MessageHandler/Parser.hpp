#pragma once
#include "FileTransfer.pb.h"
#include <string>
#include <array>
#include <functional>
constexpr static size_t bufferSize = 1024*16;
using ReadBuffer = std::array<char, bufferSize>;
class Parser
{
    private:
        std::string m_buffer;
        std::array<std::function<void(const file_transfer::Message&)>, 15> m_handlers;
    public:
        Parser() = default;
        void appendData(const char* data, size_t length);
        bool tryParseMessage();
        void appendDataAndParse(const char* data, size_t length);
        void setHandler(int messageType, std::function<void(const file_transfer::Message&)> handler);
};

