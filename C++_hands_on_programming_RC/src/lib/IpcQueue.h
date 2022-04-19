#ifndef IPCQUEUE_H
#define IPCQUEUE_H
#include "IpcCopyFile.h"
#include <mqueue.h>


class Queue : public virtual copyFilethroughIPC
{
    protected:
        unsigned int queuePriority_ = 5;
        mqd_t queueFd_ = -1;
        std::string name_ = "/CopyDataThroughQueue";
        struct mq_attr queueAttrs_;
        long mq_maxmsg_ = 10;
        long mq_msgsize_ = bufferSize_;

    public:
        virtual ~Queue() =0;
        mqd_t getQueueDescriptor();

};

class QueueSendFile : public Queue, public Reader
{
    public:
        ~QueueSendFile();
        QueueSendFile(int maxAttempt, AllToolBox* myToolBox);
        QueueSendFile(AllToolBox* myToolBox):QueueSendFile(30, myToolBox){};
        void syncIPCAndBuffer(void *data, size_t &data_size_bytes);
        void syncIPCAndBuffer(){return syncIPCAndBuffer(buffer_.data(), bufferSize_);};

};
class QueueReceiveFile : public Queue, public Writer
{
    public:
        ~QueueReceiveFile();
        QueueReceiveFile(int maxAttempt, AllToolBox* myToolBox);
        QueueReceiveFile(AllToolBox* myToolBox):QueueReceiveFile(30, myToolBox){};
        void syncIPCAndBuffer(void *data, size_t &data_size_bytes);
        void syncIPCAndBuffer()
        {
            syncIPCAndBuffer(buffer_.data(), bufferSize_);
        };

};



#endif /* IPCQUEUE_H */