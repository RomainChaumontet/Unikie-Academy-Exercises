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
    
    mq_unlink(name_.c_str());
}

QueueSendFile::QueueSendFile(int maxAttempt, toolBox* myToolBox)
{
    toolBox_ = myToolBox;
    maxAttempt_ = maxAttempt;
    int attempt = 0;
    if (queueFd_ != -1)
    {
        throw ipc_exception(
            "Error, trying to open a queue which is already opened by this program./n"
        );
    }

    do
    {
        queueFd_ = mq_open(name_.c_str(), O_RDONLY);
        if (queueFd_ == -1 && errno != ENOENT)
        {
            
            throw ipc_exception(
                "Error executing command mq_open. Errno:"
                + std::string(strerror(errno))
                );
        }
        else if (queueFd_ == -1 && errno == ENOENT)
        {
            std::cout << "Waiting to the ipc_receivefile." << std::endl;
            nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);
        }
        else 
        {
            mq_attr queueAttrs;
            mq_getattr(queueFd_, &queueAttrs);
            if (queueAttrs.mq_curmsgs > 0) // queue with message on it-> thrown
            {
                mq_unlink(name_.c_str());
                throw ipc_exception("Error. A queue some messages already exists.\n");
            }
        }
    }
    while (queueFd_ == -1 && errno == ENOENT && attempt++ < maxAttempt);
    if (attempt >= maxAttempt)
    {
        throw ipc_exception(
                "Error, can't connect to ipc_receivefile."
                );
    }

    queueFd_ = mq_open(name_.c_str(), O_WRONLY);
    if (queueFd_ == -1)
    {
        throw ipc_exception(
            "Error executing command mq_open. Errno:"
            + std::string(strerror(errno))
            );
    }
}

void QueueSendFile::syncIPCAndBuffer(void *data, size_t &data_size_bytes)
{
    struct timespec waitingtime;
    if (clock_gettime(CLOCK_REALTIME, &waitingtime) == -1)
    {
        throw ipc_exception("Error getting time");
    }
    waitingtime.tv_sec += maxAttempt_;
    if (    mq_timedsend(
                queueFd_,
                static_cast<const char*>(data),
                data_size_bytes,
                queuePriority_,
                &waitingtime
                )
            == -1)
    {   
        if (errno == ETIMEDOUT)
        {
            throw ipc_exception("Error. Can't find ipc_receivefile. Did it crash ?\n");
        }
        throw ipc_exception(
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



QueueReceiveFile::QueueReceiveFile(int maxAttempt, toolBox* myToolBox)
{
    toolBox_ = myToolBox;
    buffer_.resize(defaultBufferSize_);
    maxAttempt_ = maxAttempt;
    queueAttrs_.mq_maxmsg = mq_maxmsg_;
    queueAttrs_.mq_msgsize = mq_msgsize_;

    mq_unlink(name_.c_str());
    queueFd_ = mq_open(name_.c_str(), O_RDONLY | O_CREAT | O_EXCL,S_IRWXG |S_IRWXU, &queueAttrs_);
    if (queueFd_ == -1)
    {
        throw ipc_exception(
            "Error executing command mq_open. Errno:"
            + std::string(strerror(errno))
            );
    }
}


void QueueReceiveFile::syncIPCAndBuffer(void *data, size_t &data_size_bytes)
{ 
    if (queueFd_ == -1)
    {
        throw ipc_exception(
            "Error. Trying to sync buffer with a queue that is not opened."
        );
    }
    ssize_t amountOfData;
    struct timespec waitingtime;
    if (clock_gettime(CLOCK_REALTIME, &waitingtime) == -1)
    {
        throw ipc_exception("Error getting time");
    }
    waitingtime.tv_sec += maxAttempt_;
    amountOfData = mq_timedreceive(queueFd_,static_cast<char*>(data), mq_msgsize_, &queuePriority_,&waitingtime);
    //std::cout << "temp[0]" << temp[0] << std::endl;
    if (amountOfData == -1)
    {
        if (errno == ETIMEDOUT)
            {
                throw ipc_exception("Error. Can't find ipc_sendfile. Did it crash ?\n");
            }
        throw ipc_exception(
            "Error when trying to receive message. Errno:"
            + std::string(strerror(errno))
        );
    }
    data_size_bytes = amountOfData;
}