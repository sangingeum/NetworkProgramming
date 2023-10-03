#include <asio.hpp>
#include <fstream>
#include <iostream>
#include <array>
#include <string>

// server
using namespace asio::ip;
//127.0.0.1
asio::io_service service;
tcp::socket tcpSocket{ service };
tcp::endpoint tcp_endpoint{ tcp::v4(), 13579 };
tcp::acceptor tcpAccepter{ service , tcp_endpoint };
std::ifstream file2;
std::basic_ifstream<char8_t, std::char_traits<char8_t>> file;
std::array<char8_t, 4096> data;


void writeHandler(const asio::error_code& code, size_t bytesTransferred) {
	if (!code) {
		if (file) {
			file.read(data.data(), data.size());
			size_t bytesRead = file.gcount();
			if (bytesRead > 0)
				tcpSocket.async_write_some(asio::buffer(data, bytesRead), writeHandler);
			else
				tcpSocket.shutdown(tcp::socket::shutdown_send);
		}
	}
	else {
		tcpSocket.shutdown(tcp::socket::shutdown_send);
	}
}

void acceptHandler(const asio::error_code& code) {
	if (!code) {
		if (file) {
			file.read(data.data(), data.size());
			size_t bytesRead = file.gcount();
			if(bytesRead > 0)
				tcpSocket.async_write_some(asio::buffer(data, bytesRead), writeHandler);
			else
				tcpSocket.shutdown(tcp::socket::shutdown_send);
		}
		else {
			tcpSocket.shutdown(tcp::socket::shutdown_send);
		}
	}
}


int main() {
	file.open("fromServer.txt", std::ios::_Nocreate | std::ios::binary);
	if (file) {
		tcpAccepter.listen();
		tcpAccepter.async_accept(tcpSocket, acceptHandler);
		service.run();
	}
	else {
		std::cout << "Cannot open fromServer.txt\n" << "\n";
	}
	std::cin.get();
	return 0;
}