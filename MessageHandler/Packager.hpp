#pragma once
#include "FileTransfer.pb.h"
#include <string>

class Packager
{
    public:
        static std::string packageMessage(const file_transfer::Message& message) {
            static const int headerSize = 4; // 4 bytes for message length
            std::string serializedMessage = message.SerializeAsString();
            int messageLength = static_cast<int>(serializedMessage.size());
            std::string packagedMessage;
            packagedMessage.resize(headerSize + messageLength);
            // Prepend the length of the message as a 4-byte header
            *reinterpret_cast<int*>(const_cast<char*>(packagedMessage.data())) = messageLength;
            // Append the serialized message
            std::copy(serializedMessage.begin(), serializedMessage.end(), packagedMessage.begin() + 4);
            return packagedMessage;
        }
};

