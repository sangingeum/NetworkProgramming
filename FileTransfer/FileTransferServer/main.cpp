#include "FileTransferServer.hpp"
#include <iostream>
int main() {
	try {
		// This asynchronous file transfer server listens on port 13579.
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