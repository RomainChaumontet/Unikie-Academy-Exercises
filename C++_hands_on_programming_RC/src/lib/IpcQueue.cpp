#include "IpcQueue.h"
#include <string.h>
#include <mqueue.h>
#include <unistd.h>

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

void QueueSendFile::openIPC()
{
    queueAttrs_.mq_maxmsg = mq_maxmsg_;
    queueAttrs_.mq_msgsize = bufferSize_;
    
    mq_unlink(queueName_.c_str());

    queueFd_ = mq_open(queueName_.c_str(), O_WRONLY | O_CREAT | O_EXCL,S_IRWXG |S_IRWXU, &queueAttrs_);
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
                &buffer_[0],
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
    mq_unlink(queueName_.c_str());
}

void QueueReceiveFile::openIPC()
{
    int attempt = 0;

    do
    {
        queueFd_ = mq_open(queueName_.c_str(), O_RDONLY);
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
            sleep(1);
        }
    }
    while (queueFd_ == -1 && errno == ENOENT && ++attempt < maxAttempt_);
    
    if (attempt == maxAttempt_)
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
    amountOfData = mq_receive(queueFd_,&buffer_[0], mq_msgsize_, &queuePriority_);
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