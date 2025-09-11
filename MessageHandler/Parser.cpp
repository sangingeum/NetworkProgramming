#include "Parser.hpp"

void Parser::appendData(const char* data, size_t length) {
    m_buffer.append(reinterpret_cast<const char*>(data), length);
}

bool Parser::tryParseMessage() {
    static const int headerSize = 4; // Assuming a 4-byte header for message length

    if (m_buffer.size() <= headerSize) {
        return false;
    }

    // Extract the message length from the header
    int messageLength = *reinterpret_cast<const int*>(m_buffer.data());

    if (m_buffer.size() < headerSize + messageLength) {
        return false;
    }

    // If we reach here, we have a complete message
    file_transfer::Message message;
    if (message.ParseFromString(m_buffer.substr(headerSize, messageLength))) {
        // Clear the m_buffer after successful parsing
        m_buffer.erase(0, headerSize + messageLength);
        // Handle the parsed message
        m_handlers[message.content_case()](message);
        return true;
    }

    // If parsing fails, we might not have a complete message yet
    return false;
}

void Parser::appendDataAndParse(const char* data, size_t length) {
    appendData(data, length);
    while (tryParseMessage()) {
        // Keep trying to parse messages as long as possible
    }
}

void Parser::setHandler(int messageType, std::function<void(const file_transfer::Message&)> handler) {
    if (messageType >= 0 && messageType < m_handlers.size()) {
        m_handlers[messageType] = handler;
    }
}
