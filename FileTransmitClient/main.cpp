#include <asio.hpp>
#include <fstream>
#include <iostream>
#include <array>

// client
using namespace asio::ip;
//127.0.0.1
asio::io_service service;
tcp::socket tcpSocket{ service };
std::array<char8_t, 4096> data;
//std::ofstream file;
std::basic_ofstream<char8_t, std::char_traits<char8_t>> file;

void readHandler(const asio::error_code& code, size_t bytesTransferred) {

	if (!code) {
		file.write(data.data(), bytesTransferred);
		tcpSocket.async_read_some(asio::buffer(data, data.size()), readHandler);
	}
	
}

void connectHandler(const asio::error_code& code) {
	if (!code) {
		tcpSocket.async_read_some(asio::buffer(data, data.size()), readHandler);
	}
}

void resolverHandler(const asio::error_code& code, tcp::resolver::iterator it) {
	if (!code) {
		tcpSocket.async_connect(*it, connectHandler);
	}
}


int main() {
	file.open("fromClient.txt", std::ios::trunc | std::ios::binary);
	if (file) {
		tcp::resolver resolver{ service };
		tcp::resolver::query query{ "127.0.0.1", "13579" };
		resolver.async_resolve(query, resolverHandler);
		service.run();
		std::cin.get();
	}
	else {
		std::cout << "cannot open file\n";
	}
	
	return 0;
}