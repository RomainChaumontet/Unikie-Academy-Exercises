#ifndef IPCQUEUE_H
#define IPCQUEUE_H

#include "Tools.h"


volatile extern std::atomic_bool Rwaiting;

class SendQueueHandler : public IpcHandler
{
    private:
        HandyFunctions* myToolBox_;
        Reader myFileHandler_;
        mqd_t queueFd_ = -1;
        std::string queueName_;
        struct mq_attr queueAttrs_;
        long mq_maxmsg_ = 10;
        long mq_msgsize_;


    public:
        SendQueueHandler(HandyFunctions* toolBox, const std::string &QueueName, const std::string &filepath);
        virtual ~SendQueueHandler();
        virtual void connect() override;
        virtual void sendData(void* data, size_t data_size_bytes);
        virtual size_t transferHeader() override;
        virtual size_t transferData(std::vector<char>& buffer) override;
};


class ReceiveQueueHandler : public IpcHandler
{
    private:
        HandyFunctions* myToolBox_;
        Writer myFileHandler_;
        mqd_t queueFd_ = -1;
        std::string queueName_;
        struct mq_attr queueAttrs_;
        long mq_maxmsg_ = 10;


    public:
        ReceiveQueueHandler(HandyFunctions* toolBox, const std::string &QueueName, const std::string &filepath);
        virtual ~ReceiveQueueHandler();
        virtual void connect() override;
        virtual size_t receiveData(void* data, size_t bufferSize);
        virtual size_t transferHeader() override;
        virtual size_t transferData(std::vector<char> &buffer) override;
};




#endif