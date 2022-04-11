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
        QueueSendFile(int maxAttempt);
        QueueSendFile():QueueSendFile(30){};
        void syncIPCAndBuffer();

};
class QueueReceiveFile : public Queue, public Writer
{
    public:
        ~QueueReceiveFile();
        QueueReceiveFile(int maxAttempt);
        QueueReceiveFile():QueueReceiveFile(30){};
        void syncIPCAndBuffer();

};



#endif /* IPCQUEUE_H */