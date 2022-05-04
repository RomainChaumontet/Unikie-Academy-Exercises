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

class semaphoreHandler
{
    private:
        std::string semName_;
        sem_t* semPtr_ = SEM_FAILED;
        handyFunctions* myToolBox_;
        struct timespec ts_;
    
    public:
        semaphoreHandler(const std::string &name, handyFunctions* toolBox):semName_(name), myToolBox_(toolBox){};
        ~semaphoreHandler();
        void semCreate();
        void semConnect(std::string& PrintElements);
        void semPost();
        void semWait(bool print=false, const std::string& message = "");
};

class sharedMemoryHandler
{
    handyFunctions* myToolBox_;
    int shmFileDescriptor_ = -1;
    std::string shmName_;
    char* bufferPtr = nullptr;
    size_t shmSize_ ;
    ShmData shm_;

    public:
        sharedMemoryHandler(handyFunctions* toolBox, const std::string &ShmName);
        ~sharedMemoryHandler();
        void shmCreate();
        void shmConnect();
        ShmData& shmGetShmStruct();
};

class sendShmHandler : public ipcHandler
{
    private:
        handyFunctions* myToolBox_;
        Reader myFileHandler_;
        std::string filepath_;
        std::string shmName_;
        semaphoreHandler senderSem_;
        semaphoreHandler receiverSem_;
        sharedMemoryHandler myShm_;

    public:
        sendShmHandler(handyFunctions* toolBox, const std::string &ShmName, const std::string &filepath);
        ~sendShmHandler(){};
        virtual void connect() override;
        virtual size_t transferHeader() override;
        virtual size_t transferData(std::vector<char>& buffer) override;

};

class receiveShmHandler : public ipcHandler
{
    private:
        handyFunctions* myToolBox_;
        Writer myFileHandler_;
        std::string filepath_;
        std::string shmName_;
        semaphoreHandler senderSem_;
        semaphoreHandler receiverSem_;
        sharedMemoryHandler myShm_;

    public:
        receiveShmHandler(handyFunctions* toolBox, const std::string &ShmName, const std::string &filepath);
        ~receiveShmHandler(){};
        virtual void connect() override;
        virtual size_t transferHeader() override;
        virtual size_t transferData(std::vector<char>& buffer) override;

};

#endif