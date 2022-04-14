
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>   
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <sched.h>
#include <cstring>
#include "IpcCopyFile.h"
#include "IpcShm.h"
#include <thread>


using namespace std::chrono_literals;

Shm::~Shm(){}

ShmSendFile::ShmSendFile(int maxAttempt)
{
    maxAttempt_ = maxAttempt;
    //Opening shared memory

    shmFileDescriptor_ = shm_open(name_.c_str(), O_RDWR | O_CREAT, 0660);
    if (shmFileDescriptor_ == -1)
    {
        throw ipc_exception(
            "ShmSendFile(). Error trying to open the shared memory. Errno"
            + std::string(strerror(errno))
        );
    }

    if (ftruncate(shmFileDescriptor_,shmSize_)==-1)
    {
        throw ipc_exception("Error when setting the size of the shared memory.");
    }

    bufferPtr = static_cast<char *> (mmap(NULL, shmSize_,PROT_READ | PROT_WRITE, MAP_SHARED, shmFileDescriptor_, 0));
    if (bufferPtr == static_cast<char *>(MAP_FAILED))
    {
        throw ipc_exception(
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
    senderSemaphorePtr_ = sem_open(semSName_.c_str(), O_CREAT , S_IRWXU | S_IRWXG, 0);
    if (senderSemaphorePtr_ == SEM_FAILED)
    {
        throw ipc_exception(
            "ShmSendFile(). Error when creating the semaphore. Errno"
            + std::string(strerror(errno))
        );
    }
    receiverSemaphorePtr_ = sem_open(semRName_.c_str(), O_CREAT , S_IRWXU | S_IRWXG, 0);
    if (receiverSemaphorePtr_ == SEM_FAILED)
    {
        throw ipc_exception(
            "ShmSendFile(). Error when creating the semaphore. Errno"
            + std::string(strerror(errno))
        );
    }
}

ShmSendFile::~ShmSendFile()
{
    file_.close();
    close(shmFileDescriptor_);
    
    
    munmap(bufferPtr, shmSize_);
    

    sem_close(senderSemaphorePtr_);
    sem_close(receiverSemaphorePtr_);
    

    sem_unlink(semSName_.c_str());
    sem_unlink(semRName_.c_str());

    shm_unlink(name_.c_str());
}


//To avoid copying the data in the buffer
//The method syncFileWithBuffer is overloaded and
// the syncIPCAndBuffer method is not used.
// Or the SHM is used has the buffer



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
        throw ipc_exception("syncFileWithIPC(). Eofbit error.");
    }
    if (state == std::ios_base::failbit)
    {
        throw ipc_exception("syncFileWithIPC(). Failbit error.");

    }
    if (state == std::ios_base::badbit)
    {
        throw ipc_exception("syncFileWithIPC(). badbit error.");
    }
}

void ShmSendFile::syncFileWithIPC(const std::string &filepath)
{
    struct timespec ts;
    openFile(filepath);
    size_t dataSent = 0;

    //wait for the receiver to connect
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        throw ipc_exception("Error getting time");
    }
    ts.tv_sec += maxAttempt_;
    if (sem_timedwait(senderSemaphorePtr_,&ts) == -1) 
    {
        if (errno == ETIMEDOUT)
                throw ipc_exception("Error. Can't find ipc_receivefile. Did it crash ?\n");
        throw ipc_exception("Error, can't connect to ipc_receivefile.\n");
    }
    //sending header
    Header header(filepath, defaultBufferSize_);
    std::memcpy(shm_.data, header.getHeader().data(), defaultBufferSize_);
    shm_.main->data_size = defaultBufferSize_;

    if(sem_post(receiverSemaphorePtr_) == -1)
    {
        throw ipc_exception(
            "ShmSendFile::syncFileWithIPC(). Error when waiting the semaphore. Errno"
            + std::string(strerror(errno))
        );
    }

    //sending data

    do
    {
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        {
            throw ipc_exception("Error getting time");
        }
        ts.tv_sec += maxAttempt_;
        if (sem_timedwait(senderSemaphorePtr_,&ts) == -1)
        {
            if (errno == ETIMEDOUT)
                throw ipc_exception("Error. Can't find ipc_receivefile. Did it crash ?\n");
                
            throw ipc_exception(
                "ShmSendFile::syncFileWithIPC(). Error when waiting the semaphore. Errno"
                + std::string(strerror(errno))
            );
        }

        syncFileWithBuffer(shm_.data);
        shm_.main->data_size = bufferSize_;
        dataSent += bufferSize_;
        if(sem_post(receiverSemaphorePtr_) == -1)
        {
            throw ipc_exception(
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
    maxAttempt_ = maxAttempt;
    int tryNumber = 0;
    //Connecting to the semaphore
    senderSemaphorePtr_ = sem_open(semSName_.c_str(), O_RDWR);
    while (senderSemaphorePtr_ == SEM_FAILED) //the semaphore is not opened
    {
        std::cout << "Waiting for ipc_sendfile." << std::endl;
        std::this_thread::sleep_for(500ms);
        if (++tryNumber > maxAttempt)
        {
            throw ipc_exception(
                "Error, can't connect to the other program.\n"
                );
        }
        senderSemaphorePtr_ = sem_open(semSName_.c_str(), O_RDWR);
    }

    receiverSemaphorePtr_ = sem_open(semRName_.c_str(), O_RDWR);
    while (receiverSemaphorePtr_ == SEM_FAILED) //the semaphore is not opened
    {
        std::cout << "Waiting for ipc_senfile." << std::endl;
        std::this_thread::sleep_for(500ms);
        if (++tryNumber > maxAttempt)
        {
            throw ipc_exception(
                "Error, can't connect to the other program.\n"
                );
        }
        receiverSemaphorePtr_ = sem_open(semRName_.c_str(), O_RDWR);
    }
    //Open shared memory
    shmFileDescriptor_ = shm_open(name_.c_str(), O_RDWR, 0);
    if(shmFileDescriptor_ == -1)
    {
        throw ipc_exception(
            "ShmReceiveFile(). Error trying to open the shared memory. Errno: "
            + std::string(strerror(errno))
            );
    }

    bufferPtr = static_cast<char *>(mmap(NULL, shmSize_, PROT_READ | PROT_WRITE, MAP_SHARED, shmFileDescriptor_, 0));
    if (bufferPtr == (char*)MAP_FAILED)
    {
        throw ipc_exception(
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



void ShmReceiveFile::syncFileWithBuffer(char* bufferPtr)
{
    if (!file_.is_open())
    {
        throw ipc_exception("syncFileWithBuffer(). Error, trying to write to a file which is not opened.");
    }
    
    file_.write(bufferPtr, bufferSize_);

    auto state = file_.rdstate();
    if (state == std::ios_base::goodbit)
        return;

    if (state == std::ios_base::failbit)
    {
        throw ipc_exception("syncFileWithBuffer(). Failbit error. May be set if construction of sentry failed.");
    }
    if (state == std::ios_base::badbit)
    {
        throw ipc_exception("Writer syncFileWithBuffer(). Badbit error.");
    }
    throw ipc_exception("Writer syncFileWithBuffer(). Unknown error.");
}

void ShmReceiveFile::syncFileWithIPC(const std::string &filepath)
{
    struct timespec ts;
    openFile(filepath);
    size_t  dataReceived = 0;

    sem_post(senderSemaphorePtr_); //letting the sender send some data

    //receiving header
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        throw ipc_exception("Error getting time");
    }
    ts.tv_sec += maxAttempt_;
    if(sem_timedwait(receiverSemaphorePtr_, &ts)==-1)
    {
        if (errno == ETIMEDOUT)
            throw ipc_exception("Error. Can't find ipc_sendfile. Did it crash ?\n");

        throw ipc_exception(
            "ShmReceiveFile::syncFileWithIPC(). Error when waiting for the semaphore. Errno: "
            + std::string(strerror(errno))
            );
    }

    Header header(defaultBufferSize_);
    std::vector<size_t> headerReceived;
    headerReceived.resize(defaultBufferSize_);
    std::memcpy(headerReceived.data(),shm_.data,defaultBufferSize_);
    if (header.getHeader()[0] != headerReceived[0])
    {
        throw ipc_exception("Error. Another message is present. Maybe another program uses this IPC.\n");
    }
    fileSize_ = headerReceived[1];

    sem_post(senderSemaphorePtr_);


    do
    {
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        {
            throw ipc_exception("Error getting time");
        }
        ts.tv_sec += maxAttempt_;
        if(sem_timedwait(receiverSemaphorePtr_, &ts)==-1)
        {
            if (errno == ETIMEDOUT)
                throw ipc_exception("Error. Can't find ipc_sendfile. Did it crash ?\n");

            throw ipc_exception(
                "ShmReceiveFile::syncFileWithIPC(). Error when waiting for the semaphore. Errno: "
                + std::string(strerror(errno))
                );
        }

        bufferSize_ = shm_.main->data_size;
        syncFileWithBuffer(shm_.data);
        dataReceived += bufferSize_;
        sem_post(senderSemaphorePtr_);
    } while (bufferSize_ > 0 && dataReceived < fileSize_);

    file_.close();
    if (fileSize_ != returnFileSize(filepath))
    {
        throw ipc_exception("Error, filesize mismatch. Maybe another program uses the IPC.\n");
    }

}

