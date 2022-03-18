#ifndef IPCQUEUE_H
#define IPCQUEUE_H
#include "IpcCopyFile.h"
#include <mqueue.h>


class Queue : public virtual copyFilethroughIPC
{
    protected:
        unsigned int queuePriority_ = 5;
        mqd_t queueFd_ = -1;
        struct mq_attr queueAttrs_;
        long mq_maxmsg_ = 1024;
        long mq_msgsize_ = bufferSize_;

    public:
        virtual void openQueue() = 0;
        virtual void syncQueueAndBuffer() =0;
        virtual ~Queue() =0;

};

class QueueSendFile : public Queue, public Reader
{
    public:
        ~QueueSendFile();
        void openQueue();
        void syncQueueAndBuffer();

};



#endif /* IPCQUEUE_H */