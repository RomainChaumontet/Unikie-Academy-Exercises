#ifndef IPCSHM_H
#define IPCSHM_H

#include "Tools.h"


volatile extern std::atomic_bool SemWaiting;

class ShmData_Header
{
    public:
        unsigned init_flag;
        size_t data_size;
};

class ShmData
{
    public:
        ShmData_Header* main;
        char* data;
};

class SemaphoreHandler
{
    private:
        std::string semName_;
        sem_t* semPtr_ = SEM_FAILED;
        HandyFunctions* myToolBox_;
        struct timespec ts_;
    
    public:
        SemaphoreHandler(const std::string &name, HandyFunctions* toolBox):semName_(name), myToolBox_(toolBox){};
        ~SemaphoreHandler();
        void semCreate();
        void semConnect(std::string& PrintElements);
        void semPost();
        void semWait(bool print=false, const std::string& message = "");
};

class SharedMemoryHandler
{
    HandyFunctions* myToolBox_;
    int shmFileDescriptor_ = -1;
    std::string shmName_;
    char* bufferPtr = nullptr;
    size_t shmSize_ ;
    ShmData shm_;

    public:
        SharedMemoryHandler(HandyFunctions* toolBox, const std::string &ShmName);
        ~SharedMemoryHandler();
        void shmCreate();
        void shmConnect();
        ShmData& shmGetShmStruct();
};

class SendShmHandler : public IpcHandler
{
    private:
        HandyFunctions* myToolBox_;
        Reader myFileHandler_;
        std::string filepath_;
        std::string shmName_;
        SemaphoreHandler senderSem_;
        SemaphoreHandler receiverSem_;
        SharedMemoryHandler myShm_;

    public:
        SendShmHandler(HandyFunctions* toolBox, const std::string &ShmName, const std::string &filepath);
        ~SendShmHandler(){};
        virtual void connect() override;
        virtual size_t transferHeader() override;
        virtual size_t transferData(std::vector<char>& buffer) override;

};

class ReceiveShmHandler : public IpcHandler
{
    private:
        HandyFunctions* myToolBox_;
        Writer myFileHandler_;
        std::string filepath_;
        std::string shmName_;
        SemaphoreHandler senderSem_;
        SemaphoreHandler receiverSem_;
        SharedMemoryHandler myShm_;

    public:
        ReceiveShmHandler(HandyFunctions* toolBox, const std::string &ShmName, const std::string &filepath);
        ~ReceiveShmHandler(){};
        virtual void connect() override;
        virtual size_t transferHeader() override;
        virtual size_t transferData(std::vector<char>& buffer) override;

};

#endif