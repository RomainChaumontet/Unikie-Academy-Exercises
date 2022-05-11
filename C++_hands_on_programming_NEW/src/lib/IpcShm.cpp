#include "IpcShm.h"

#pragma region semaphoreHandler

volatile std::atomic_bool SemWaiting;

semaphoreHandler::~semaphoreHandler()
{
    if (semPtr_ != SEM_FAILED)
        sem_close(semPtr_);
    sem_unlink(semName_.c_str());
}

void semaphoreHandler::semCreate()
{
    semPtr_ = sem_open(semName_.c_str(), O_CREAT , S_IRWXU | S_IRWXG, 0);
    if (semPtr_ == SEM_FAILED)
    {
        throw ipc_exception(
            "ShmSendFile(). Error when creating the semaphore. Errno"
            + std::string(strerror(errno))
        );
    }
}

void semaphoreHandler::semConnect(std::string& PrintElements)
{
    int numberOfTries = 0;
    
    semPtr_ = sem_open(semName_.c_str(), O_RDWR);
    while (semPtr_ == SEM_FAILED) //the semaphore is not opened
    {
        myToolBox_->updatePrintingElements(PrintElements);
        myToolBox_->nap(10);
        if (++numberOfTries > myToolBox_->getMaxAttempt()*100)
        {
            throw ipc_exception(
                "Error, can't connect to the other program.\n"
                );
        }
        semPtr_ = sem_open(semName_.c_str(), O_RDWR);
    }
}

void semaphoreHandler::semPost()
{
    int status = sem_post(semPtr_);
    if (status == -1)
    {
        throw ipc_exception("Error when trying to post the semaphore: " + semName_);
    }
}


void semaphoreHandler::semWait(bool print, const std::string& message)
{
    myToolBox_->getTime(ts_);
    ts_.tv_sec += myToolBox_->getMaxAttempt();
    int status;

    if (print)
    {
        SemWaiting = true;

        auto printwaiting = std::async(std::launch::async, [&]()
        {
            HandyFunctions toolBox;
            while(SemWaiting)
            {
                toolBox.updatePrintingElements(message);
                toolBox.nap(10);
            }
        });

        status = sem_timedwait(semPtr_, &ts_);
        SemWaiting = false;
        printwaiting.get();
    }
    else
    {
        status = sem_timedwait(semPtr_, &ts_);
    }


    
    if(status==-1)
    {
        if (errno == ETIMEDOUT)
            throw ipc_exception("Error. Can't find ipc_sendfile. Did it crash ?\n");

        throw ipc_exception(
            "ShmReceiveFile::syncFileWithIPC(). Error when waiting for the semaphore. Errno: "
            + std::string(strerror(errno))
            );
    }
}

#pragma endregion semaphoreHandler

#pragma region sharedMemoryHandler

sharedMemoryHandler::sharedMemoryHandler(
    HandyFunctions* toolBox,
    const std::string &ShmName
    ): myToolBox_(toolBox),
    shmName_(ShmName),
    shmSize_(sizeof(ShmData_Header) + toolBox->getDefaultBufferSize())
{}

sharedMemoryHandler::~sharedMemoryHandler()
{
    close(shmFileDescriptor_);
    munmap(bufferPtr, shmSize_);
    shm_unlink(shmName_.c_str());
}

void sharedMemoryHandler::shmCreate()
{
    shmFileDescriptor_ = shm_open(shmName_.c_str(), O_RDWR | O_CREAT, 0660);
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
}

void sharedMemoryHandler::shmConnect()
{
    std::cout << "Try to open shared memory with the name " << shmName_ << std::endl;
    shmFileDescriptor_ = shm_open(shmName_.c_str(), O_RDWR, 0);
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

ShmData& sharedMemoryHandler::shmGetShmStruct()
{
    return shm_;
}

#pragma endregion sharedMemoryHandler

#pragma region sendShmHandler

sendShmHandler::sendShmHandler(
    HandyFunctions* toolBox,
    const std::string &ShmName,
    const std::string &filepath
    ):myToolBox_(toolBox),
    myFileHandler_(filepath, toolBox),
    filepath_(filepath),
    shmName_(ShmName),
    senderSem_(toolBox->getSemName(ShmName).senderSemaphoreName, toolBox),
    receiverSem_(toolBox->getSemName(ShmName).receiverSemaphoreName, toolBox),
    myShm_(toolBox,ShmName)
{}

void sendShmHandler::connect()
{
    myShm_.shmCreate();
    std::cout << "Shared memory created" << std::endl;
    senderSem_.semCreate();
    receiverSem_.semCreate();
    std::cout << "Semaphores created" << std::endl;
    senderSem_.semWait(true, "Waiting for ipc_receivefile");
}

size_t sendShmHandler::transferHeader()
{
    Header header(myToolBox_->getKey(),myToolBox_->returnFileSize(filepath_), myToolBox_);
    std::memcpy(myShm_.shmGetShmStruct().data, header.getData(), myToolBox_->getDefaultBufferSize());
    myShm_.shmGetShmStruct().main->data_size = myToolBox_->getDefaultBufferSize();
    receiverSem_.semPost();

    return header.getFileSize();
}

size_t sendShmHandler::transferData(std::vector<char>& buffer)
{
    senderSem_.semWait();
    size_t dataRead = myFileHandler_.readFile(myShm_.shmGetShmStruct().data, myToolBox_->getDefaultBufferSize());
    myShm_.shmGetShmStruct().main->data_size = dataRead;
    receiverSem_.semPost();

    return dataRead;
}

#pragma endregion sendShmHandler

#pragma region receiveShmHandler

receiveShmHandler::receiveShmHandler(
    HandyFunctions* toolBox,
    const std::string &ShmName,
    const std::string &filepath
    ):myToolBox_(toolBox),
    myFileHandler_(filepath, toolBox),
    filepath_(filepath),
    shmName_(ShmName),
    senderSem_(toolBox->getSemName(ShmName).senderSemaphoreName, toolBox),
    receiverSem_(toolBox->getSemName(ShmName).receiverSemaphoreName, toolBox),
    myShm_(toolBox,ShmName)
{}

void receiveShmHandler::connect()
{
    std::string message = "Waiting for ipc_sendfile.";
    receiverSem_.semConnect(message);
    senderSem_.semConnect(message);
    std::cout << "Connected to semaphores" << std::endl;
    myShm_.shmConnect();
    std::cout << "Connected to the shared memory" << std::endl;
    senderSem_.semPost();
}

size_t receiveShmHandler::transferHeader()
{
    Header header(myToolBox_->getKey(),myToolBox_);
    std::vector<size_t> headerReceived;
    headerReceived.resize(myToolBox_->getDefaultBufferSize());
    
    receiverSem_.semWait();

    if ( myShm_.shmGetShmStruct().main->data_size != myToolBox_->getDefaultBufferSize())
    {
        throw ipc_exception("Error. Another message is present. Maybe another program uses this IPC.\n");
    }

    std::memcpy(headerReceived.data(),myShm_.shmGetShmStruct().data,myToolBox_->getDefaultBufferSize());
    
    if (header.getKey() != headerReceived[0])
    {
        throw ipc_exception("Error. Another message is present. Maybe another program uses this IPC.\n");
    }

    senderSem_.semPost();
    return headerReceived[1];
}

size_t receiveShmHandler::transferData(std::vector<char>& buffer)
{
    receiverSem_.semWait();
    size_t dataReceived = myShm_.shmGetShmStruct().main->data_size;
    myFileHandler_.writeFile(myShm_.shmGetShmStruct().data, dataReceived);
    myShm_.shmGetShmStruct().main->data_size = 0;
    senderSem_.semPost();

    return dataReceived;
}

#pragma endregion receiveShmHandler