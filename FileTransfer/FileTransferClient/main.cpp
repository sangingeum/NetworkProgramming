#include "FileTransferClient.hpp"
#include <iostream>

int main() {
	try {
		asio::io_service service;
		FileTransferClient client{ service };
		client.asyncGetFile("client.txt", "127.0.0.1", "13579");
		service.run();
	}
	catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
	}
	return 0;
}