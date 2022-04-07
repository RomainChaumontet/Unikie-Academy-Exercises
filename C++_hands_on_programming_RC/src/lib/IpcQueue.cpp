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
    
    mq_unlink(name_.c_str());
}

QueueSendFile::QueueSendFile(int maxAttempt)
{
    maxAttempt_ = maxAttempt;
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
            std::cout << "Waiting to the ipc_receivefile." << std::endl;
            nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);
        }
    }
    while (queueFd_ == -1 && errno == ENOENT && attempt++ < maxAttempt);
    if (attempt >= maxAttempt)
    {
        throw std::runtime_error(
                "Error, can't connect to the other program."
                );
    }

    queueFd_ = mq_open(name_.c_str(), O_WRONLY);
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
    struct timespec waitingtime;
    if (clock_gettime(CLOCK_REALTIME, &waitingtime) == -1)
    {
        throw std::runtime_error("Error getting time");
    }
    waitingtime.tv_sec += maxAttempt_;
    if (    mq_timedsend(
                queueFd_,
                buffer_.data(),
                bufferSize_,
                queuePriority_,
                &waitingtime
                )
            == -1)
    {   
        if (errno == ETIMEDOUT)
        {
            throw std::runtime_error("Error. Can't find the other program. Did it crash ?\n");
        }
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
    maxAttempt_ = maxAttempt;
    queueAttrs_.mq_maxmsg = mq_maxmsg_;
    queueAttrs_.mq_msgsize = mq_msgsize_;

    
    mq_unlink(name_.c_str());

    queueFd_ = mq_open(name_.c_str(), O_RDONLY | O_CREAT | O_EXCL,S_IRWXG |S_IRWXU, &queueAttrs_);
    if (queueFd_ == -1)
    {
        throw std::runtime_error(
            "Error executing command mq_open. Errno:"
            + std::string(strerror(errno))
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
    struct timespec waitingtime;
    if (clock_gettime(CLOCK_REALTIME, &waitingtime) == -1)
    {
        throw std::runtime_error("Error getting time");
    }
    waitingtime.tv_sec += maxAttempt_;
    amountOfData = mq_timedreceive(queueFd_,buffer_.data(), mq_msgsize_, &queuePriority_,&waitingtime);
    if (amountOfData == -1)
    {
        if (errno == ETIMEDOUT)
            {
                throw std::runtime_error("Error. Can't find the other program. Did it crash ?\n");
            }
        throw std::runtime_error(
            "Error when trying to receive message. Errno:"
            + std::string(strerror(errno))
        );
    }
    bufferSize_ = amountOfData;
    buffer_.resize(bufferSize_);

}