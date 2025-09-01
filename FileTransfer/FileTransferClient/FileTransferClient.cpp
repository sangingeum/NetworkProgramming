#include "FileTransferClient.hpp"
#include <exception>
#include <format>
#include <functional>


FileTransferClient::FileTransferClient(asio::io_context& context)
	: m_context(context), m_curPath(std::filesystem::current_path())
{}

void FileTransferClient::connect(std::string_view host, std::string_view port){
	tcp::resolver::query query{ host.data() ,port.data() };
	auto resolver = std::make_shared<tcp::resolver>(m_context); 
	resolver->async_resolve(query, [this, resolver](const asio::error_code& code, tcp::resolver::iterator it) {
		resolverHandler(code, it);
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

void FileTransferClient::sendStatus(std::shared_ptr<tcp::socket> socket, bool success) {
	file_transfer::Message message;
	message.mutable_client_status()->set_success(success);
	std::string data = message.SerializeAsString();
	asio::async_write(*socket, asio::buffer(data), [](const asio::error_code& code, size_t bytesTransferred){
		if(!code){
			std::cout << "Succecssfully sent client status to the server.\n";
		}}
	);
}

void FileTransferClient::readHandler(std::shared_ptr<tcp::socket> socket, std::shared_ptr<ReadBuffer> buffer, const asio::error_code& code, size_t bytesTransferred) {
	std::cout << "In readHandler\n";
	if(!code){
		std::cout << "Received " << bytesTransferred << " bytes from the server.\n";
		file_transfer::Message message;
		message.ParseFromArray(buffer->data(), static_cast<int>(bytesTransferred));
		switch(message.content_case()){
			case file_transfer::Message::kFileList:
				std::cout << "Received file list message from the server.\n";
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
		socket->async_read_some(asio::buffer(buffer->data(), buffer->size()),
		std::bind(&FileTransferClient::readHandler, this, socket, buffer, std::placeholders::_1, std::placeholders::_2));
	}
}

void FileTransferClient::connectionSuccessHandler(std::shared_ptr<tcp::socket> socket, const asio::error_code& code) {
	auto buffer = std::make_shared<ReadBuffer>();
	std::cout << "Succecssfully connected to the server.\n";
	sendFileListRequest(socket);
	socket->async_read_some(asio::buffer(buffer->data(), buffer->size()),
	std::bind(&FileTransferClient::readHandler, this, socket, buffer, std::placeholders::_1, std::placeholders::_2));
}

void FileTransferClient::connectionFailureHandler(std::shared_ptr<tcp::socket> socket, tcp::resolver::iterator it, const asio::error_code& code){
	std::cerr << "Failed to connect to the server: " << code.message() << '\n';
	socket->async_connect(*it, [this, it, socket](const asio::error_code& code) {
		if(!code){
			connectionSuccessHandler(socket, code);
		}
		else{
			connectionFailureHandler(socket, it, code);
		}
	});
}

void FileTransferClient::resolverHandler(const asio::error_code& code, tcp::resolver::iterator it) {
	if (!code) {
		std::cout << "Succecssfully resolved the server address.\n";
		auto socket = std::make_shared<tcp::socket>(m_context);
		socket->async_connect(*it, [this, it, socket](const asio::error_code& code) {
			if(!code){
				connectionSuccessHandler(socket, code);
			}
			else{
				connectionFailureHandler(socket, it, code);
			}
		});
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