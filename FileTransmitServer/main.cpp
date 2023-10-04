#include "FileTransferServer.hpp"

int main() {
	asio::io_service service;
	FileTransferServer server{ service , 13579 };
	server.start("server.txt");
	return 0;
}