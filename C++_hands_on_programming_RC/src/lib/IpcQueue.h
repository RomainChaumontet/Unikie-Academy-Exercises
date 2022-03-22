#ifndef IPCQUEUE_H
#define IPCQUEUE_H
#include "IpcCopyFile.h"
#include <mqueue.h>


class Queue : public virtual copyFilethroughIPC
{
    protected:
        unsigned int queuePriority_ = 5;
        mqd_t queueFd_ = -1;
        std::string queueName_ = "/" + name_;
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
        void openIPC();
        void syncIPCAndBuffer();

};
class QueueReceiveFile : public Queue, public Writer
{
    protected:
        int maxAttempt_ = 60;

    public:
        ~QueueReceiveFile();
        void openIPC();
        void syncIPCAndBuffer();

};



#endif /* IPCQUEUE_H */