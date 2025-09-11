# NetworkProgramming
Network program examples written in C++ using the Asio library

## Requirements
You need the following to build this project.

    MSBuild
    C++20
    Asio library
    Protocol Buffer (protoc should be in your PATH)

If you have ```vcpkg``` installed, I recommend installing Asio and Protobuf using the following command:
```
vcpkg install protobuf protobuf:x64-windows asio
```

[Here](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-msbuild?pivots=shell-powershell)   is how to install ```vcpkg```   
Refer to [this page](https://think-async.com/Asio/) if you want to learn more about the ```Asio library```.    
See [this page](https://github.com/protocolbuffers) for more details about ```Protocol Buffers```.

## Implementations

1. FileTransfer client and server
2. Chat client and server

## How to build
 
* Make sure ```protoc``` is in your PATH.    
* Run ```generate_proto.bat``` to generate pb.h and pb.c files.    
* Build the project in ```Visual Studio``` or by using this command:   
```MSBuild .```
