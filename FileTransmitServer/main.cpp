#include <asio.hpp>
#include <fstream>
#include <iostream>
#include <array>

using namespace asio::ip;
//127.0.0.1
asio::io_service service;
tcp::socket tcpSocket{ service };
tcp::endpoint tcp_endpoint{ tcp::v4(), 13579 };
tcp::acceptor tcpAccepter{ service , tcp_endpoint };



void acceptHandler(const asio::error_code& code) {
	if (!code) {
		std::cout << "hi\n";
	}
}


int main() {

	tcpAccepter.listen();
	tcpAccepter.async_accept(tcpSocket, acceptHandler);

	service.run();

	return 0;
}