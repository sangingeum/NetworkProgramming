#pragma once
#include "FileTransfer.pb.h"
#include <string>
#include <cstring>

class Packager
{
    public:
        static std::string packageMessage(const file_transfer::Message& message);
};

