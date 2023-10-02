#include <asio.hpp>
#include <fstream>
#include <iostream>
#include <array>

using namespace asio::ip;
//127.0.0.1
asio::io_service service;
tcp::socket tcpSocket{ service };



void connectHandler(const asio::error_code& code) {
	if (!code) {
		std::cout << "hi\n";
	}
	std::cout << code.message();
}

void resolverHandler(const asio::error_code& code, tcp::resolver::iterator it) {
	if (!code) {
		tcpSocket.async_connect(*it, connectHandler);
	}
}


int main() {
	tcp::resolver resolver{ service };
	tcp::resolver::query query{ "127.0.0.1", "13579" };
	resolver.async_resolve(query, resolverHandler);

	service.run();

	return 0;
}