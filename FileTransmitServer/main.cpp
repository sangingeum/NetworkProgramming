#include "FileTransferServer.hpp"

int main() {
	asio::io_service service;
	FileTransferServer server{ service , 13579 };
	server.asyncSendFile("fromServer.txt");
	service.run();
	return 0;
}