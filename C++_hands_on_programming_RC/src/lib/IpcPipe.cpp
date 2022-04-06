#include "IpcPipe.h"
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <thread>


using namespace std::chrono_literals;

Pipe::~Pipe(){};

PipeSendFile::~PipeSendFile()
{
    if (pipeFile_.is_open())
    {
        pipeFile_.close();
    }
    if (checkIfFileExists(name_))
    {
        unlink(name_.c_str());
    }
}

PipeSendFile::PipeSendFile()
{
    if (pipeFile_.is_open())
    {
        throw std::runtime_error(
            "Error, trying to create a new pipe whereas the program is already connected to one."
        );
    }

    unlink(name_.c_str());

    if (mkfifo(name_.c_str(),S_IRWXU | S_IRWXG) == -1)
    {
        throw std::runtime_error(
            "Error when trying to create the pipe. Errno:"
            + std::string(strerror(errno))
        );
    }

    pipeFile_.open(name_, std::ios::out | std::ios::binary);
    if (!pipeFile_.is_open())
    {
        throw std::runtime_error(
            "Error when trying to connect to the pipe. rdstate:" + file_.rdstate());
    }
}


void PipeSendFile::syncIPCAndBuffer()
{
    if (!pipeFile_.is_open())
    {
        throw std::runtime_error("syncIPCAndBuffer(). Error, trying to write to a pipe which is not opened.");
    }

    pipeFile_.write(buffer_.data(), bufferSize_);

    auto state = pipeFile_.rdstate();
    if (state == std::ios_base::goodbit)
        return;

    if (state == std::ios_base::failbit)
    {
        throw std::runtime_error("syncIPCAndBuffer(). Failbit error. May be set if construction of sentry failed.");
    }
    if (state == std::ios_base::badbit)
    {
        throw std::runtime_error("syncIPCAndBuffer(). Badbit error.");
    }
    throw std::runtime_error("syncIPCAndBuffer(). Unknown error.");
}


PipeReceiveFile::~PipeReceiveFile()
{
    if (pipeFile_.is_open())
    {
        pipeFile_.close();
    }
    if (checkIfFileExists(name_))
    {
        unlink(name_.c_str());
    }
}

PipeReceiveFile::PipeReceiveFile()
{
    int count = 0;
    while (!checkIfFileExists(name_) && count++ < 60)
    {
        std::cout << "Waiting for ipc_sendfile."<<std::endl;
        std::this_thread::sleep_for (500ms);
    }

    if (pipeFile_.is_open())
    {
        throw std::runtime_error(
            "Error, trying to open a pipe that is already opened."
        );
    }

    pipeFile_.open(name_, std::ios::in | std::ios::binary);
    if (!pipeFile_.is_open())
    {
        throw std::runtime_error(
            "Error when trying to connect to the pipe. rdstate:" + file_.rdstate());
    }
}

void PipeReceiveFile::syncIPCAndBuffer()
{
    if (!pipeFile_.is_open())
    {
        throw std::runtime_error("Error, trying to read in a pipe that is not opened");
    }

    std::vector<char> (bufferSize_).swap(buffer_);

    pipeFile_.read(buffer_.data(), bufferSize_);
    bufferSize_ = pipeFile_.gcount();
    buffer_.resize(bufferSize_);

    auto state = pipeFile_.rdstate();
    if (state == std::ios_base::goodbit)
        return;
    if (state == std::ios_base::eofbit+std::ios_base::failbit)
        return; // end of file
    if (state == std::ios_base::eofbit)
    {
        throw std::runtime_error("syncIPCAndBuffer(). Eofbit error.");
        return;
    }
    if (state == std::ios_base::failbit)
    {
        throw std::runtime_error("syncIPCAndBuffer(). Failbit error.");
        return;
    }
    if (state == std::ios_base::badbit)
    {
        throw std::runtime_error("syncIPCAndBuffer(). badbit error.");
        return;
    }

}
