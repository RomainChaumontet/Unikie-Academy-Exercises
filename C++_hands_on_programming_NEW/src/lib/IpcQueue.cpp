#include "IpcCopyFile.h"
#include "IpcQueue.h"

#pragma region sendQueueHandler
sendQueueHandler::sendQueueHandler(
    handyFunctions* toolBox,
    const std::string &QueueName,
    const std::string &filepath
    ):myToolBox_(toolBox),
    myFileHandler_(filepath, toolBox),
    queueName_(QueueName)
{};

sendQueueHandler::~sendQueueHandler()
{
    mq_close(queueFd_);
    mq_unlink(queueName_.c_str());
}


void sendQueueHandler::connect()
{
    std::cout << "Connecting to a queue with the name: " << queueName_ << std::endl;
    int attempt = 0;
    do
    {
        queueFd_ = mq_open(queueName_.c_str(), O_WRONLY);

        if (queueFd_ != -1)
        {
            mq_getattr(queueFd_, &queueAttrs_);
            if (queueAttrs_.mq_curmsgs > 0) // queue with message on it-> thrown
            {
                mq_close(queueFd_);
                mq_unlink(queueName_.c_str());
                throw ipc_exception("Error. A queue with some messages already exists.\n");
            }
            else
                break;
        }
        if (++attempt > myToolBox_->getMaxAttempt()*100)
            throw ipc_exception("Error, can't connect to ipc_receivefile.\n");
        if (errno != ENOENT)
        {
            throw ipc_exception("Error executing command mq_open. Errno:"
                + std::string(strerror(errno)) + "\n");
        }
        
        // Queue has not been opened yet
        myToolBox_->updatePrintingElements("Waiting to the ipc_receivefile.");
        myToolBox_->nap(10);
    } while (1);
}

void sendQueueHandler::sendData(void* data, size_t data_size_bytes)
{
    struct timespec waitingtime;
    myToolBox_->getTime(waitingtime);
    waitingtime.tv_sec += myToolBox_->getMaxAttempt();

    if ( mq_timedsend(
        queueFd_,
        static_cast<const char*>(data),
        data_size_bytes, 0,
        &waitingtime
        )
        == -1)
    {
        if(errno == ETIMEDOUT)
        {
            throw ipc_exception("Error. Can't find ipc_receivefile. Did it crash ?\n");
        }
        throw ipc_exception(
            "Error executing command mq_send. Errno:"
            + std::string(strerror(errno))
            + "\n"
            );
    }
}

size_t sendQueueHandler::transferHeader()
{
    size_t fileSize = myFileHandler_.fileSize();
    Header myHeader(myToolBox_->getKey(), fileSize, myToolBox_);

    sendData(myHeader.getData(), myToolBox_->getDefaultBufferSize());

    return fileSize;
}

size_t sendQueueHandler::transferData(std::vector<char>& buffer)
{
    size_t bufferSize = myToolBox_->getDefaultBufferSize();
    buffer.resize(bufferSize);
    size_t dataRead = myFileHandler_.readFile(buffer.data(), bufferSize);
    sendData(buffer.data(), dataRead);

    return dataRead;
}

#pragma endregion sendQueueHandler


#pragma region receiveQueueHandler
receiveQueueHandler::receiveQueueHandler(
    handyFunctions* toolBox,
    const std::string &QueueName,
    const std::string &filepath
    ):myToolBox_(toolBox),
    myFileHandler_(filepath, toolBox),
    queueName_(QueueName)
{};

void receiveQueueHandler::connect()
{
    queueAttrs_.mq_maxmsg = mq_maxmsg_;
    queueAttrs_.mq_msgsize = myToolBox_->getDefaultBufferSize();

    std::cout << "Creating a queue with the name: " << queueName_ << std::endl;
    queueFd_ = mq_open(queueName_.c_str(), O_RDONLY | O_CREAT,S_IRWXG |S_IRWXU, &queueAttrs_);
    if (queueFd_ == -1)
    {
        throw ipc_exception(
            "Error creating the queue. Errno:"
            + std::string(strerror(errno))
            );
    }
}

receiveQueueHandler::~receiveQueueHandler()
{
    mq_close(queueFd_);
    mq_unlink(queueName_.c_str());
}

size_t receiveQueueHandler::receiveData(void* data, size_t bufferSize)
{
    int sizeReceived;
    struct timespec waitingtime;
    myToolBox_->getTime(waitingtime);
    waitingtime.tv_sec += myToolBox_->getMaxAttempt();

    sizeReceived = mq_timedreceive(queueFd_, static_cast<char*>(data), myToolBox_->getDefaultBufferSize(), 0, &waitingtime);

    if (sizeReceived == -1)
    {
        Rwaiting = false;
        if (errno == ETIMEDOUT)
        {
            throw ipc_exception("Error. Can't find ipc_sendfile. Did it crash ?\n");
        }
        throw ipc_exception(
            "Error when trying to receive message. Errno:"
            + std::string(strerror(errno))
            + "\n"
        );
    }
    
    return sizeReceived;
}

volatile std::atomic_bool Rwaiting;

size_t receiveQueueHandler::transferHeader()
{
    Header myHeader(myToolBox_->getKey(), myToolBox_);

    Rwaiting = true;
    auto printwaiting = std::async(std::launch::async, [&]()
    {
        handyFunctions toolBox;
        while(Rwaiting)
        {
            toolBox.updatePrintingElements("Waiting for ipc_sendfile.");
            toolBox.nap(10);
        }
    });
    size_t HeaderSize = receiveData(myHeader.getData(),myToolBox_->getDefaultBufferSize());
    Rwaiting = false;
    printwaiting.get();
    if(HeaderSize < 2*sizeof(size_t) || myHeader.getKey() != myToolBox_->getKey())
    {
        throw ipc_exception("Error, incorrect header found.\n");
    }

    return myHeader.getFileSize();
}

size_t receiveQueueHandler::transferData(std::vector<char> &buffer) 
{
    size_t bufferSize = myToolBox_->getDefaultBufferSize();
    buffer.resize(bufferSize);
    size_t sizeReadfromPipe = receiveData(buffer.data(), bufferSize);
    myFileHandler_.writeFile(buffer.data(), sizeReadfromPipe);
    return sizeReadfromPipe;
}


#pragma endregion receiveQueueHandler