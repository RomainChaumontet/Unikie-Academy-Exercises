#include "IpcQueue.h"
#include <string.h>

QueueSendFile::~QueueSendFile()
{
    if (queueFd_ != -1)
    {
        if (mq_close(queueFd_) == -1)
        {
            throw std::runtime_error(
                "Error executing command mq_close. Errno: "
                + std::string(strerror(errno))
                );
        }        
    }
}

void QueueSendFile::openQueue()
{
    queueAttrs_.mq_maxmsg = mq_maxmsg_;
    queueAttrs_.mq_msgsize = bufferSize_;

    queueFd_ = mq_open(name_.c_str(), O_RDONLY, O_CREAT, &queueAttrs_);

    if (queueFd_ == -1)
    {
        throw std::runtime_error(
            "Error executing command mq_open. Errno:"
            + std::string(strerror(errno))
            );
    }
}

void QueueSendFile::syncQueueAndBuffer()
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