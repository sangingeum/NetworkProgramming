#include "FileTransferServer.hpp"
#include <iostream>
int main() {
	try {
		asio::io_service service;
		FileTransferServer server{ service , 13579 };
		server.start("server.txt");
	}
	catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
	}
	return 0;
}