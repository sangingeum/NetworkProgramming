#include "ChatClient.hpp"
#include <iostream>

// Release mode
int main() {
	asio::io_service service;
	ChatClient client{ service, "Keum" };

	if (client.connect("127.0.0.1", "13579"))
		client.send("good morning");
	else
		std::cout << "Error\n";

	return 0;
}