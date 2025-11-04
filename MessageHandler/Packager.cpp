#include "Packager.hpp"

std::string Packager::packageMessage(const file_transfer::Message& message) {
    static constexpr int headerSize = 4; // 4 bytes for message length
    std::string serializedMessage = message.SerializeAsString();
    int messageLength = static_cast<int>(serializedMessage.size());
    std::string packagedMessage;
    packagedMessage.resize(headerSize + messageLength);
    // Prepend the length of the message as a 4-byte header
    std::memcpy((void*)packagedMessage.data(), (void*)&messageLength, headerSize);
    // Append the serialized message
    std::copy(serializedMessage.begin(), serializedMessage.end(), packagedMessage.begin() + headerSize);
    return packagedMessage;
}
