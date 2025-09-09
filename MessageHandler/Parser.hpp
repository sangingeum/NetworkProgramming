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
        void appendData(const char* data, size_t length) {
            buffer.append(data, length);
        }

        bool tryParseMessage() {
            static const int headerSize = 4; // Assuming a 4-byte header for message length
            
            if (buffer.size() <= headerSize) {
                return false;
            }

            // Extract the message length from the header
            int messageLength = *reinterpret_cast<const int*>(buffer.data());

            if (buffer.size() < headerSize + messageLength) {
                return false;
            }

            // If we reach here, we have a complete message
            file_transfer::Message message;
            if (message.ParseFromString(buffer.substr(headerSize, messageLength))) {
                // Clear the buffer after successful parsing
                buffer.erase(0, headerSize + messageLength);
                // Handle the parsed message
                handlers[message.content_case()](message);
                return true;
            }

            // If parsing fails, we might not have a complete message yet
            return false;
        }

        void appendDataAndParse(const char* data, size_t length) {
            appendData(data, length);
            while(tryParseMessage()) {
                // Keep trying to parse messages as long as possible
            }
        }

        void setHandler(int messageType, std::function<void(const file_transfer::Message&)> handler) {
            if (messageType >= 0 && messageType < handlers.size()) {
                handlers[messageType] = handler;
            }
        }
};

