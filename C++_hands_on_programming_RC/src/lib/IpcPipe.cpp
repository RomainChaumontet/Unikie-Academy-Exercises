#include "IpcPipe.h"
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <future>
#include <thread>
#include <chrono>


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

    auto future = std::make_unique<std::future<bool>>(std::async(std::launch::async, [&](){
        pipeFile_.open(name_, std::ios::out | std::ios::binary);
        return !pipeFile_.is_open();
    }));

    std::future_status status;
    status = future->wait_for(2s);
    switch(status)
    {
        case std::future_status::deferred:
        {
            future.release(); //the destructor of async shall not be called. It is an intentional memory leak but as the program will end, it's ok.
            throw std::runtime_error("Inconsistencies : got deferred status while using std::async\n");
            break;
        }
        case std::future_status::timeout:
        {
            future.release(); //the destructor of async shall not be called. It is an intentional memory leak but as the program will end, it's ok.
            throw std::runtime_error("Error, can't connect to the other program.");
            break;
        }
        case std::future_status::ready:
        {
            if (future->get()==false)
                throw std::runtime_error("Error opening the pipe. rdstate:" + file_.rdstate());
            break;
        }   
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

PipeReceiveFile::PipeReceiveFile(int maxAttempt)
{
    maxAttempt_ = maxAttempt;
    int count = 0;
    while (!checkIfFileExists(name_) && count++ < maxAttempt)
    {
        std::cout << "Waiting for ipc_sendfile."<<std::endl;
        nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);
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
