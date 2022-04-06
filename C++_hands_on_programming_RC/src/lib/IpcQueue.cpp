#include "IpcQueue.h"
#include <string.h>
#include <mqueue.h>
#include <unistd.h>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;
Queue::~Queue(){}

mqd_t Queue::getQueueDescriptor()
{
    return queueFd_;
}

QueueSendFile::~QueueSendFile()
{
    if (queueFd_ != -1)
    {
        mq_close(queueFd_);   
    }
}

QueueSendFile::QueueSendFile()
{
    if (queueFd_ != -1)
    {
        throw std::runtime_error(
            "Error, trying to open a queue which is already opened by this program./n"
        );
    }
    queueAttrs_.mq_maxmsg = mq_maxmsg_;
    queueAttrs_.mq_msgsize = mq_msgsize_;
    
    mq_unlink(name_.c_str());

    queueFd_ = mq_open(name_.c_str(), O_WRONLY | O_CREAT | O_EXCL,S_IRWXG |S_IRWXU, &queueAttrs_);
    if (queueFd_ == -1)
    {
        throw std::runtime_error(
            "Error executing command mq_open. Errno:"
            + std::string(strerror(errno))
            );
    }
}

void QueueSendFile::syncIPCAndBuffer()
{
    if (    mq_send(
                queueFd_,
                buffer_.data(),
                bufferSize_,
                queuePriority_
                )
            == -1)
    {
        throw std::runtime_error(
            "Error executing command mq_send. Errno:"
            + std::string(strerror(errno))
            );
    }
}

QueueReceiveFile::~QueueReceiveFile()
{
    if (queueFd_ != -1)
    {
        mq_close(queueFd_);   
    }
    mq_unlink(name_.c_str());
}



QueueReceiveFile::QueueReceiveFile(int maxAttempt)
{
    int attempt = 0;
    if (queueFd_ != -1)
    {
        throw std::runtime_error(
            "Error, trying to open a queue which is already opened by this program./n"
        );
    }
    do
    {
        queueFd_ = mq_open(name_.c_str(), O_RDONLY);
        if (queueFd_ == -1 && errno != ENOENT)
        {
            
            throw std::runtime_error(
                "Error executing command mq_open. Errno:"
                + std::string(strerror(errno))
                );
        }
        else if (queueFd_ == -1 && errno == ENOENT)
        {
            std::cout << "Waiting to the ipcsendfile." << std::endl;
            std::this_thread::sleep_for (500ms);
        }
    }
    while (queueFd_ == -1 && errno == ENOENT && ++attempt < maxAttempt);
    
    if (attempt == maxAttempt)
    {
        throw std::runtime_error(
                "Error, the queue was not opened by another process"
                );
    }
}


void QueueReceiveFile::syncIPCAndBuffer()
{
    if (queueFd_ == -1)
    {
        throw std::runtime_error(
            "Error. Trying to sync buffer with a queue that is not opened."
        );
    }
    std::vector<char> (mq_msgsize_).swap(buffer_);
    ssize_t amountOfData;
    amountOfData = mq_receive(queueFd_,buffer_.data(), mq_msgsize_, &queuePriority_);
    if (amountOfData == -1)
    {
        throw std::runtime_error(
            "Error when trying to receive message. Errno:"
            + std::string(strerror(errno))
        );
    }
    bufferSize_ = amountOfData;
    buffer_.resize(bufferSize_);

}