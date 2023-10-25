#include "FileTransferServer.hpp"
#include <iostream>
int main() {
	try {
		// This asynchronous file transfer server listens on port 13579.
		// This server continuously transfers the 'server.txt' file to any clients
		// that connect to it using the Asio library.
		asio::io_service service;
		FileTransferServer server{ service , 13579 };
		server.start("server.txt");
	}
	catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
	}
	return 0;
}