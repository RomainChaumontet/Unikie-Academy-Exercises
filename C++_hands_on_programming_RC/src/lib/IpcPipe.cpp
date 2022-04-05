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
#include <pthread.h>
#include <signal.h>


using namespace std::chrono_literals;
struct ThreadInfo
{
    pthread_t thread1;
    pthread_t timer;
    std::fstream* pipFilePtr;
    std::string pipeName;
    int waitingTime;
};

void* TimerThread(void* arg)
{
    ThreadInfo* info = static_cast<ThreadInfo*>(arg);
    int attempt = 0;
    while (attempt++ < info->waitingTime*2)
    {
        nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);
    }
    pthread_cancel(info->thread1);

    return nullptr;
}

void* OpenThread(void* arg)
{
    ThreadInfo* info = static_cast<ThreadInfo*>(arg);
    info->pipFilePtr->open(info->pipeName,std::ios::out | std::ios::binary);
    std::cout << "Pipe is opened in both sides." << std::endl;
    pthread_cancel(info->timer);
    return nullptr;
}

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

PipeSendFile::PipeSendFile(int maxAttempt)
{
    maxAttempt_ = maxAttempt;   
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

    ThreadInfo* info =  new ThreadInfo;
    info->pipeName = name_;
    info->pipFilePtr = &pipeFile_;
    info->waitingTime = maxAttempt_;

    ::pthread_create(&info->thread1, nullptr,&OpenThread, (void*)info);
    ::pthread_create(&info->timer, nullptr,&TimerThread, (void*)info);
    
    ::pthread_join(info->timer, nullptr);
    void* retval;
    ::pthread_join(info->thread1, &retval); 
    if (retval == PTHREAD_CANCELED)
    {
        unlink(name_.c_str());
        throw std::runtime_error("Error, can't connect to the other program.\n" );

    }
    if (!pipeFile_.is_open())
    {
        throw std::runtime_error(
            "Error opening the pipe.\n"
        );
    }

    delete(info);
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

    if (count >= maxAttempt)
    {
        throw std::runtime_error("Error, can't connect to the other program.\n" );
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
