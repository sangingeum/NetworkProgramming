#pragma once
#include "Chat.pb.h"
#include <string_view>
#include <bitset>
class ChatMessage
{
private:
	uint32_t m_size{ 0 };
	std::string m_serialization{ "" };
	Chat m_data;
public:
	static const constexpr uint32_t headerSize = 4;
	static const constexpr uint32_t maxLength = 1024;
	ChatMessage() = default;
	ChatMessage(std::string_view name, std::string_view content)
	{
		set(name, content);
	}
	void set(std::string_view name, std::string_view content) {
		m_data.set_name(name.data());
		m_data.set_content(content.data());
		std::string temp = m_data.SerializeAsString();
		uint32_t length = temp.size() + headerSize;
		char header[4]{ 0, 0, 0, 0 };
		std::memcpy(header, &length, headerSize);
		std::string headerString(header, header + headerSize);
		m_serialization = headerString + temp;
		m_size = m_serialization.size();
	}

	std::string serialize() const {
		return m_serialization;
	}

	const std::string& getName() const {
		return m_data.name();
	}

	const std::string& getContent() const {
		return m_data.content();
	}

	uint32_t getSize() const {
		return m_size;
	}

	// DeSerialize and shorten the given string
	// It returns true on success
	bool deSerialize(std::string& serialization) {
		if (serialization.size() < headerSize)
			return false;
		uint32_t tempSize;
		std::memcpy(&tempSize, serialization.data(), headerSize);
		if (tempSize > maxLength || serialization.size() < tempSize)
			return false;
		std::string body{ serialization.begin() + headerSize, serialization.begin() + tempSize };
		bool success = m_data.ParseFromString(body);
		if (!success)
			return false;
		m_serialization = serialization;
		m_size = tempSize;
		serialization.erase(0, tempSize);
		return true;
	}

	// DeSerialize the given string
	// It returns true on success
	bool deSerialize(std::string_view serialization) {
		if (serialization.size() < headerSize)
			return false;
		uint32_t tempSize;
		std::memcpy(&tempSize, serialization.data(), headerSize);
		if (tempSize > maxLength || serialization.size() < tempSize)
			return false;
		std::string body{ serialization.begin() + headerSize, serialization.begin() + tempSize };
		bool success = m_data.ParseFromString(body);
		if (!success)
			return false;
		m_serialization = serialization;
		m_size = tempSize;
		return true;
	}

};

