#include "FileTransferClient.hpp"

int main() {
	asio::io_service service;
	FileTransferClient client{ service };
	client.asyncGetFile("client.txt", "127.0.0.1", "13579");
	service.run();
	return 0;
}