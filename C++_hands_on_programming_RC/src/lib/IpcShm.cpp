
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>   
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <sched.h>
#include "IpcCopyFile.h"
#include "IpcShm.h"

Shm::~Shm(){}

ShmSendFile::ShmSendFile(int maxAttempt)
{
    maxAttempt_ = maxAttempt;
    //Opening shared memory
    shm_unlink(name_.c_str());

    shmFileDescriptor_ = shm_open(name_.c_str(), O_RDWR | O_CREAT, 0660);
    if (shmFileDescriptor_ == -1)
    {
        throw std::runtime_error(
            "ShmSendFile(). Error trying to open the shared memory. Errno"
            + std::string(strerror(errno))
        );
    }

    if (ftruncate(shmFileDescriptor_,shmSize_)==-1)
    {
        throw std::runtime_error("Error when setting the size of the shared memory.");
    }

    bufferPtr = static_cast<char *> (mmap(NULL, shmSize_,PROT_READ | PROT_WRITE, MAP_SHARED, shmFileDescriptor_, 0));
    if (bufferPtr == static_cast<char *>(MAP_FAILED))
    {
        throw std::runtime_error(
            "ShmSendFile(). Error when mapping the memory. Errno"
            + std::string(strerror(errno))
        );
    }

    shm_.main = reinterpret_cast<ShmData_Header*>(bufferPtr);
    shm_.data = static_cast<char *>(bufferPtr+sizeof(ShmData_Header));
    shm_.main->data_size = 0;
    shm_.main->init_flag = 1;

    close(shmFileDescriptor_);

    //Creating semaphores
    sem_unlink(semSName_.c_str());
    senderSemaphorePtr_ = sem_open(semSName_.c_str(), O_CREAT , S_IRWXU | S_IRWXG, 0);
    if (senderSemaphorePtr_ == SEM_FAILED)
    {
        throw std::runtime_error(
            "ShmSendFile(). Error when creating the semaphore. Errno"
            + std::string(strerror(errno))
        );
    }
    sem_unlink(semRName_.c_str());
    receiverSemaphorePtr_ = sem_open(semRName_.c_str(), O_CREAT , S_IRWXU | S_IRWXG, 0);
    if (receiverSemaphorePtr_ == SEM_FAILED)
    {
        throw std::runtime_error(
            "ShmSendFile(). Error when creating the semaphore. Errno"
            + std::string(strerror(errno))
        );
    }
}

ShmSendFile::~ShmSendFile()
{
    file_.close();
    if (shmFileDescriptor_ != -1)
    {
        close(shmFileDescriptor_);
    }
    
    munmap(bufferPtr, shmSize_);
    

    if (senderSemaphorePtr_ != SEM_FAILED)
    {
        sem_close(senderSemaphorePtr_);
    }
    if (receiverSemaphorePtr_ != SEM_FAILED)
    {
        sem_close(receiverSemaphorePtr_);
    }

    sem_unlink(semSName_.c_str());
    sem_unlink(semRName_.c_str());
}


//To avoid copying the data in the buffer
//The method syncFileWithBuffer is overloaded and
// the syncIPCAndBuffer method is not used.
// Or the SHM is used has the buffer

void ShmSendFile::syncIPCAndBuffer(){};


void ShmSendFile::syncFileWithBuffer(char* bufferPtr)
{
    
    // read data in shm
    file_.read(bufferPtr,bufferSize_);
    bufferSize_ = file_.gcount();

    auto state = file_.rdstate();
    if (state == std::ios_base::goodbit)
        return;
    if (state == std::ios_base::eofbit+std::ios_base::failbit)
        return; // end of file
    if (state == std::ios_base::eofbit)
    {
        throw std::runtime_error("syncFileWithIPC(). Eofbit error.");
    }
    if (state == std::ios_base::failbit)
    {
        throw std::runtime_error("syncFileWithIPC(). Failbit error.");

    }
    if (state == std::ios_base::badbit)
    {
        throw std::runtime_error("syncFileWithIPC(). badbit error.");
    }
}

void ShmSendFile::syncFileWithIPC(const std::string &filepath)
{
    struct timespec ts;
    openFile(filepath);


    //wait for the receiver to connect
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        throw std::runtime_error("Error getting time");
    }
    ts.tv_sec += maxAttempt_;
    if (sem_timedwait(senderSemaphorePtr_,&ts) == -1) 
    {
        throw std::runtime_error("Error, can't connect to the other program.\n");
    }
    sem_post(senderSemaphorePtr_);

    do
    {
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        {
            throw std::runtime_error("Error getting time");
        }
        ts.tv_sec += 1;
        if (sem_timedwait(senderSemaphorePtr_,&ts) == -1)
        {
            throw std::runtime_error(
                "ShmSendFile::syncFileWithIPC(). Error when waiting the semaphore. Errno"
                + std::string(strerror(errno))
            );
        }

        syncFileWithBuffer(shm_.data);
        shm_.main->data_size = bufferSize_;

        if(sem_post(receiverSemaphorePtr_) == -1)
        {
            throw std::runtime_error(
                "ShmSendFile::syncFileWithIPC(). Error when waiting the semaphore. Errno"
                + std::string(strerror(errno))
            );
        }
    }
    while (bufferSize_ > 0);
}



//////////////////// ShmReceiveFile ///////////////
ShmReceiveFile :: ShmReceiveFile(int maxAttempt)
{
    int tryNumber = 0;
    //Connecting to the semaphore
    senderSemaphorePtr_ = sem_open(semSName_.c_str(), O_RDWR);
    while (senderSemaphorePtr_ == SEM_FAILED) //the semaphore is not opened
    {
        std::cout << "Waiting for ipc_senfile." << std::endl;
        nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);
        if (++tryNumber > maxAttempt)
        {
            throw std::runtime_error(
                "Error, can't connect to the other program.\n"
                );
        }
        senderSemaphorePtr_ = sem_open(semSName_.c_str(), O_RDWR);
    }

    receiverSemaphorePtr_ = sem_open(semRName_.c_str(), O_RDWR);
    while (receiverSemaphorePtr_ == SEM_FAILED) //the semaphore is not opened
    {
        std::cout << "Waiting for ipc_senfile." << std::endl;
        nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);
        if (++tryNumber > maxAttempt*2)
        {
            throw std::runtime_error(
                "Error, can't connect to the other program.\n"
                );
        }
        receiverSemaphorePtr_ = sem_open(semRName_.c_str(), O_RDWR);
    }
    //Open shared memory
    shmFileDescriptor_ = shm_open(name_.c_str(), O_RDWR, 0);
    if(shmFileDescriptor_ == -1)
    {
        throw std::runtime_error(
            "ShmReceiveFile(). Error trying to open the shared memory. Errno: "
            + std::string(strerror(errno))
            );
    }

    bufferPtr = static_cast<char *>(mmap(NULL, shmSize_, PROT_READ | PROT_WRITE, MAP_SHARED, shmFileDescriptor_, 0));
    if (bufferPtr == (char*)MAP_FAILED)
    {
        throw std::runtime_error(
            "ShmReceiveFile(). Error when mapping the memory. Errno"
            + std::string(strerror(errno))
        );
    }

    shm_.main = reinterpret_cast<ShmData_Header*>(bufferPtr);
    shm_.data = static_cast<char *>(bufferPtr+sizeof(ShmData_Header));

    close(shmFileDescriptor_);
}

ShmReceiveFile::~ShmReceiveFile()
{
    file_.close();
    if (shmFileDescriptor_ != -1)
    {
        close(shmFileDescriptor_);
    }
    
    munmap(bufferPtr, shmSize_);

    if (senderSemaphorePtr_ != SEM_FAILED)
    {
        sem_close(senderSemaphorePtr_);
    }
    if (receiverSemaphorePtr_ != SEM_FAILED)
    {
        sem_close(receiverSemaphorePtr_);
    }

    shm_unlink(name_.c_str());
}

void ShmReceiveFile::syncIPCAndBuffer(){}


void ShmReceiveFile::syncFileWithBuffer(char* bufferPtr)
{
    if (!file_.is_open())
    {
        throw std::runtime_error("syncFileWithBuffer(). Error, trying to write to a file which is not opened.");
    }
    
    file_.write(bufferPtr, bufferSize_);

    auto state = file_.rdstate();
    if (state == std::ios_base::goodbit)
        return;

    if (state == std::ios_base::failbit)
    {
        throw std::runtime_error("syncFileWithBuffer(). Failbit error. May be set if construction of sentry failed.");
    }
    if (state == std::ios_base::badbit)
    {
        throw std::runtime_error("Writer syncFileWithBuffer(). Badbit error.");
    }
    throw std::runtime_error("Writer syncFileWithBuffer(). Unknown error.");
}

void ShmReceiveFile::syncFileWithIPC(const std::string &filepath)
{
    struct timespec ts;
    openFile(filepath);

    sem_post(senderSemaphorePtr_); //letting the sender send some data

    do
    {
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        {
            throw std::runtime_error("Error getting time");
        }
        ts.tv_sec += 1;
        if(sem_timedwait(receiverSemaphorePtr_, &ts)==-1)
        {
            throw std::runtime_error(
                "ShmReceiveFile::syncFileWithIPC(). Error when waiting for the semaphore. Errno: "
                + std::string(strerror(errno))
                );
        }

        bufferSize_ = shm_.main->data_size;
        syncFileWithBuffer(shm_.data);

        sem_post(senderSemaphorePtr_);
    } while (bufferSize_ > 0);
}

