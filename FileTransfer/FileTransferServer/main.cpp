#include "FileTransferServer.hpp"
#include <iostream>
int main() {
	try {
		// This asynchronous file transfer server listens on port 13579.
		// This server continuously transfers the 'server.txt' file to any clients
		// that connect to it using the Asio library.
		asio::io_context context;
		FileTransferServer server{ context , 13579 };
		server.start();
		context.run();
	}
	catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
	}
	return 0;
}