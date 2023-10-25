#include "FileTransferClient.hpp"
#include <iostream>

int main() {
	try {
		// This asynchronous file transfer client tries to connect to the endpoint ("127.0.0.1", "13579").
		// After it receives a file from the server, it terminates.
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