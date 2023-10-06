#include "ChatClient.hpp"
#include <iostream>

// Release mode
int main() {
	asio::io_service service;
	ChatClient client{ service, "Keum" };

	if (client.connect("127.0.0.1", "13579"))
		while (true) {
			std::string content;
			std::getline(std::cin, content, '\n');
			client.send(content);
		}
	else
		std::cout << "Error\n";

	return 0;
}