#ifndef IPCSHM_H
#define IPCSHM_H

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>   
#include <semaphore.h>

class ShmData_Header
{
    public:
        unsigned init_flag;
        int data_size;
};

class ShmData
{
    public:
        ShmData_Header* main;
        char* data;
};


class Shm : public virtual copyFilethroughIPC
{
    public:
        virtual ~Shm();

    protected:
        std::string name_ = "CopyDataThroughSharedMemory";
        int shmFileDescriptor_ = -1;
        ShmData shm_;
        std::string semSName_ = "mySenderSemaphore";
        std::string semRName_ = "myReceiverSemaphore";
	    sem_t* senderSemaphorePtr_ = SEM_FAILED;
	    sem_t* receiverSemaphorePtr_ = SEM_FAILED;
        size_t shmSize_ = sizeof(ShmData_Header)+bufferSize_;
        char* bufferPtr;
        
};

class ShmSendFile : public Shm, public Reader
{
    public:
        ShmSendFile();
        ~ShmSendFile();
        void syncFileWithIPC(const std::string &filepath);
        void syncFileWithBuffer(char* bufferPtr);
        void syncIPCAndBuffer();

};

class ShmReceiveFile : public Shm, public Writer
{
    public:
        ShmReceiveFile(int maxAttempt);
        ShmReceiveFile():ShmReceiveFile(60){};
        ~ShmReceiveFile();
        void syncFileWithIPC(const std::string &filepath);
        void syncFileWithBuffer(char* bufferPtr);
        void syncIPCAndBuffer();

};

#endif /* IPCSHM_H */