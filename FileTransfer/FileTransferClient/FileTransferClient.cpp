#include "FileTransferClient.hpp"
#include <exception>
#include <format>
#include <functional>


FileTransferClient::FileTransferClient(asio::io_context& service)
	: m_socket(service), m_resolver(service), m_curPath(std::filesystem::current_path())
{}

void FileTransferClient::connect(std::string_view host, std::string_view port){
	tcp::resolver::query query{ host.data() ,port.data() };
	m_resolver.async_resolve(query, [this](const asio::error_code& code, tcp::resolver::iterator it) {
		resolverHandler(code, it);
	});
}

void FileTransferClient::requestFileTransfer(std::string_view fileName){
	//TODO: Check if the files already exists

	file_transfer::FileTranferRequest request;
	request.set_name(fileName.data());
	std::string data = request.SerializeAsString();
	m_socket.async_send(asio::buffer(data), [](const asio::error_code& code, size_t bytesTransferred){
		if(!code){
			std::cout << "Succecssfully sent file tranfer request to the server.\n";
		}
	});
}

void FileTransferClient::requestFileList(){
	file_transfer::FileListRequest request;
	std::string data = request.SerializeAsString();
	m_socket.async_send(asio::buffer(data), [](const asio::error_code& code, size_t bytesTransferred){
		if(!code){
			std::cout << "Succecssfully sent file list request to the server.\n";
		}}
	);
}

void FileTransferClient::sendStatus(bool success) {
	file_transfer::ClientStatus status;
	status.set_success(success);
	std::string data = status.SerializeAsString();
	m_socket.async_send(asio::buffer(data), [](const asio::error_code& code, size_t bytesTransferred){
		if(!code){
			std::cout << "Succecssfully sent client status to the server.\n";
		}}
	);
}

void FileTransferClient::connectHandler(const asio::error_code& code) {
	if (!code) {
		std::cout << "Succecssfully connected to the server.\n";
		requestFileList();
		m_socket.async_read_some(asio::buffer(m_data, m_data.size()), 
		std::bind(&FileTransferClient::readHandler, this, std::placeholders::_1, std::placeholders::_2));
	}
	else {
		std::cerr << "Failed to connect to the server: " << code.message() << '\n';
	}
}

void FileTransferClient::resolverHandler(const asio::error_code& code, tcp::resolver::iterator it) {
	if (!code) {
		m_socket.async_connect(*it, [this](const asio::error_code& code) {
			connectHandler(code);
		});
	}
}

void FileTransferClient::readHandler(const asio::error_code& code, size_t bytesTransferred) {
	if(!code){
		file_transfer::Message message;
		message.ParseFromArray(m_data.data(), static_cast<int>(bytesTransferred));
		switch(message.content_case()){
			case file_transfer::Message::kFileList:
				fileListHandler(message.file_list());
				break;
			case file_transfer::Message::kFileInfo:
				fileInfoHandler(message.file_info());
				break;
			case file_transfer::Message::kFileChunk:
				fileChunkHandler(message.file_chunk());
				break;
			case file_transfer::Message::kFileTransferComplete:
				fileTransferCompleteHandler(message.file_transfer_complete());
				break;
			case file_transfer::Message::kError:
				erorrHandler(message.error());
				break;
			default:
				std::cout << "unknown message received from the server.\n";
				break;
		}
	}
}

void FileTransferClient::fileListHandler(const file_transfer::FileList& list) {
	std::cout << "Received file list from the server:\n";
	for (const auto& file : list.files()) {
		std::cout << file.name() << ", " << file.size() << '\n';
	}
}

void FileTransferClient::fileInfoHandler(const file_transfer::FileInfo& info){

}
void FileTransferClient::fileChunkHandler(const file_transfer::FileChunk& chunk){

}
void FileTransferClient::fileTransferCompleteHandler(const file_transfer::FileTransferComplete& complete){

}
void FileTransferClient::erorrHandler(const file_transfer::Error& error){
	
}