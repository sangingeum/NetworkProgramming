#pragma once
#include "FileTransfer.pb.h"
#include <string>
#include <array>
#include <functional>
class Parser
{
    private:
        std::string buffer;
        std::array<std::function<void(const file_transfer::Message&)>, 15> handlers;
    public:
        Parser() = default;
        void appendData(const char* data, size_t length);
        bool tryParseMessage();
        void appendDataAndParse(const char* data, size_t length);
        void setHandler(int messageType, std::function<void(const file_transfer::Message&)> handler);
};

