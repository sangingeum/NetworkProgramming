# NetworkProgramming
Network program examples written in C++ using the Asio library

## Requirements
You need the following to build this project:

    MSBuild
    C++20
    Asio library
    Protocol Buffer (protoc should be in your PATH)

If you have `vcpkg` installed, you can install Asio and Protobuf using:
```
vcpkg install protobuf protobuf:x64-windows asio
```

[Here](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-msbuild?pivots=shell-powershell) is how to install `vcpkg`  
Refer to [this page](https://think-async.com/Asio/) to learn more about the `Asio library`  
See [this page](https://github.com/protocolbuffers) for more details about `Protocol Buffers`

## Implementations

### 1. FileTransfer Client and Server
An asynchronous file transfer system that allows clients to browse and download files from the server.

**Features:**
- List available files on the server
- Request and download files by name
- Chunked file transfer with progress tracking
- Error handling and recovery for failed transfers
- Support for multiple concurrent file transfers using unique identifiers

**Protocol:**
- Uses Protocol Buffers for message serialization
- Length-prefixed message framing (4-byte header)
- Messages include: FileListRequest, FileTransferRequest, FileInfo, FileChunk, FileTransferComplete, and error messages

**How to use:**
- Server listens on port 13579 and serves files from its current directory
- Client connects to `127.0.0.1:13579` and provides commands:
  - `list` - Request list of available files
  - `get <filename>` - Download a specific file
  - `connect` - Reconnect to server
  - `quit` - Exit the client

### 2. Chat Client and Server
A simple chat system (separate project in Chat folder)

## Project Structure

- `FileTransfer/` - File transfer client and server implementations
  - `FileTransferServer/` - Server that serves files from its directory
  - `FileTransferClient/` - Client with interactive command interface
- `MessageHandler/` - Shared message packaging and parsing utilities
  - `Packager` - Serializes messages with length-prefix headers
  - `Parser` - Deserializes messages and dispatches to handlers
- `idl/` - Protocol Buffer definitions
  - `FileTransfer.proto` - Message definitions for file transfer protocol
- `generated/` - Auto-generated Protocol Buffer code
- `Chat/` - Chat application (separate project)

## How to Build
 
* Make sure `protoc` is in your PATH
* Run `generate_proto.bat` to generate .pb.h and .pb.cc files
* Build the project in Visual Studio or using:
```
MSBuild .
```
