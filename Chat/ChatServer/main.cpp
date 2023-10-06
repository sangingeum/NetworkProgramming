#include "ChatServer.hpp"

int main() {
	asio::io_service service;
	ChatServer server{ service, 13579 };
	server.start();
	return 0;
}