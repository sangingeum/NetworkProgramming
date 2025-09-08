#include "FileTransferClient.hpp"
#include <exception>
#include <format>
#include <functional>
#include <random>

FileTransferClient::FileTransferClient(asio::io_context& context)
	: m_context(context), m_curPath(std::filesystem::current_path())
{}

void FileTransferClient::connect(std::string_view host, std::string_view port){
	tcp::resolver::query query{ host.data() ,port.data() };
	auto resolver = std::make_shared<tcp::resolver>(m_context); 
	resolver->async_resolve(query, [this, resolver](const asio::error_code& code, tcp::resolver::iterator it) {
		handleResolve(code, it);
	});
}

void FileTransferClient::sendFileTransferRequest(std::shared_ptr<tcp::socket> socket, std::string_view fileName){
	//TODO: Check if the files already exists
	file_transfer::Message message;
	message.mutable_file_transfer_request()->set_name(fileName.data());
	std::string data = message.SerializeAsString();
	asio::async_write(*socket, asio::buffer(data.data(), data.size()), [](const asio::error_code& code, size_t bytesTransferred){
		if(!code){
			std::cout << "Succecssfully sent file tranfer request to the server.\n";
		}
	});
}

void FileTransferClient::sendFileListRequest(std::shared_ptr<tcp::socket> socket){
	file_transfer::Message message;
	message.mutable_file_list_request();
	std::string data = message.SerializeAsString();
	asio::async_write(*socket, asio::buffer(data.data(), data.size()), [](const asio::error_code& code, size_t bytesTransferred){
		if(!code){
			std::cout << "Succecssfully sent file list request to the server.\n";
			std::cout << "bytesTransferred: " << bytesTransferred << '\n';
		}}
	);
}

void FileTransferClient::sendAcknowledgement(std::shared_ptr<tcp::socket> socket, bool success) {
	file_transfer::Message message;
	message.mutable_client_acknowledgement()->set_success(success);
	std::string data = message.SerializeAsString();
	asio::async_write(*socket, asio::buffer(data), [](const asio::error_code& code, size_t bytesTransferred){
		if(!code){
			std::cout << "Succecssfully sent client status to the server.\n";
		}}
	);
}

void FileTransferClient::sendReady(std::shared_ptr<tcp::socket> socket)
{
	file_transfer::Message message;
	message.mutable_client_ready();
	// TODO
}

void FileTransferClient::handleRead(std::shared_ptr<tcp::socket> socket, std::shared_ptr<ReadBuffer> buffer, const asio::error_code& code, size_t bytesTransferred) {
	if(!code){
		std::cout << "Received " << bytesTransferred << " bytes from the server.\n";
		file_transfer::Message message;
		message.ParseFromArray(buffer->data(), static_cast<int>(bytesTransferred));
		switch(message.content_case()){
			case file_transfer::Message::kFileList:
				std::cout << "Received file list message from the server.\n";
				handleFileList(message.file_list());
				break;
			case file_transfer::Message::kFileInfo:
				handleFileInfo(message.file_info());
				break;
			case file_transfer::Message::kFileChunk:
				handleFileChunk(message.file_chunk());
				break;
			case file_transfer::Message::kFileTransferComplete:
				handleFileTransferCompletion(message.file_transfer_complete());
				break;
			case file_transfer::Message::kError:
				handleError(message.error());
				break;
			default:
				std::cout << "unknown message received from the server.\n";
				break;
		}
		socket->async_read_some(asio::buffer(buffer->data(), buffer->size()),
		std::bind(&FileTransferClient::handleRead, this, socket, buffer, std::placeholders::_1, std::placeholders::_2));
	}
	else{
		std::cerr << "Error on receive: " << code.message() << '\n';
	}
}

void FileTransferClient::handleConnectionSuccess(std::shared_ptr<tcp::socket> socket, const asio::error_code& code) {
	auto buffer = std::make_shared<ReadBuffer>();
	std::cout << "Succecssfully connected to the server.\n";
	sendFileListRequest(socket);
	socket->async_read_some(asio::buffer(buffer->data(), buffer->size()),
	std::bind(&FileTransferClient::handleRead, this, socket, buffer, std::placeholders::_1, std::placeholders::_2));
}

void FileTransferClient::handleConnectionFailure(std::shared_ptr<tcp::socket> socket, tcp::resolver::iterator it, const asio::error_code& code){
	std::cerr << "Failed to connect to the server: " << code.message() << '\n';
	socket->async_connect(*it, [this, it, socket](const asio::error_code& code) {
		if(!code){
			handleConnectionSuccess(socket, code);
		}
		else{
			handleConnectionFailure(socket, it, code);
		}
	});
}

void FileTransferClient::handleResolve(const asio::error_code& code, tcp::resolver::iterator it) {
	if (!code) {
		std::cout << "Succecssfully resolved the server address.\n";
		auto socket = std::make_shared<tcp::socket>(m_context);
		socket->async_connect(*it, [this, it, socket](const asio::error_code& code) {
			if(!code){
				handleConnectionSuccess(socket, code);
			}
			else{
				handleConnectionFailure(socket, it, code);
			}
		});
	}
}

void FileTransferClient::handleFileList(const file_transfer::FileList& list) {
	std::cout << "Received file list from the server:\n";
	for (const auto& file : list.files()) {
		std::cout << file.name() << ", " << file.size() << '\n';
	}
}

void FileTransferClient::handleFileInfo(const file_transfer::FileInfo& info){
	std::cout << "Start downloading file: " << info.name() << " (" << info.size() << " bytes)\n";
	// make a random name
	std::string filename = generateRandomUniqueFilename();
	// open file
	// TODO
	
}
void FileTransferClient::handleFileChunk(const file_transfer::FileChunk& chunk){
	// write to file

}
void FileTransferClient::handleFileTransferCompletion(const file_transfer::FileTransferComplete& complete){
	// check file name collision
	// change file name
	// close file
}
void FileTransferClient::handleError(const file_transfer::Error& error){
	// stop reading and close file
}

// File operations
std::string FileTransferClient::generateRandomUniqueFilename(){
	static std::string_view textPool =
		"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	static std::mt19937 rng{ std::random_device{}() };
	static std::uniform_int_distribution<std::size_t> dist(0, textPool.size() - 1);

	std::string filename;
	filename.reserve(13); // Reserve space for 13 characters
	do {
		filename.clear();
		for (size_t i = 0; i < 12; ++i) { // Generate a 12-character random string
			filename += textPool[dist(rng)];
		}
		filename += ".dat"; // Add a file extension
	} while (std::filesystem::exists(m_curPath / filename)); // Ensure the filename is unique

	return filename;
}