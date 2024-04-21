#include "ChatClient.hpp"
#include <iostream>

// Since this chat client is running on a console, 
// it does not actively update the chat log. It updates the log after the user input something.
int main() {
	asio::io_service service;
	ChatClient client{ service, "Keum" };
	std::string content;
	std::cout << "Enter your name: ";
	std::getline(std::cin, content, '\n');
	client.setName(content);
	if (client.connect("127.0.0.1", "13579")) {
		std::cout << "Connected to the server\n";
		// Read messages from the server & Print them
		client.start();
	}
	else
		std::cout << "Error\n";

	return 0;
}