#include "FileTransferClient.hpp"
#include <iostream>
#include <thread>

int main() {
	try {
		// This asynchronous file transfer client tries to connect to the endpoint ("127.0.0.1", "13579").
		// After it receives a file from the server, it terminates.
		asio::io_context service;
		FileTransferClient client{ service };
		client.connect("127.0.0.1", "13579");
		
		std::thread ioThread([&service]() {
			service.run();
			}); 

		// Wait for user input
		while(true){
			std::cout << "Enter command (list, get <filename>, connect, quit): ";
			std::string command;
			std::getline(std::cin, command);
			if(command == "list"){
				auto session = client.getSession();
				if(!session){
					std::cout << "Not connected to any server.\n";
					continue;
				}
				session->sendFileListRequest();
			}
			else if(command.rfind("get ", 0) == 0){
				std::string filename = command.substr(4);
				auto session = client.getSession();
				if(!session){
					std::cout << "Not connected to any server.\n";
					continue;
				}
				session->sendFileTransferRequest(filename);
			}
			else if(command == "connect"){
				client.connect("127.0.0.1", "13579");
			}
			else if(command == "quit"){
				break;
			}
			else{
				std::cout << "Unknown command.\n";
			}
		}	

		ioThread.join();

	}
	catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
	}
	return 0;
}