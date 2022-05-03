#include "IpcCopyFile.h"
#include "IpcPipe.h"

////////////////// fifoHandler /////////////////
#pragma region fifoHandler

void fifoHandler::createFifo() const
{
    std::cout << "Creating the pipe " << pipeName_ << std::endl;
    if (mkfifo(pipeName_.c_str(),S_IRWXU | S_IRWXG) == -1 && errno != EEXIST)
    {
        throw ipc_exception(
            "Error when trying to create the pipe. Errno: "
            + std::string(strerror(errno))
        );
    }
}

fifoHandler::~fifoHandler()
{
    unlink(pipeName_.c_str());
}

#pragma endregion fifoHandler

////////////////// sendPipeHandler ////////////
#pragma region sendPipeHandler

volatile std::atomic_bool sigpipe_received;

static void sigpipe_handler(int signum)
{
    sigpipe_received = true;
}

sendPipeHandler::sendPipeHandler(
    handyFunctions* toolBox,
    const std::string &pipeName,
    const std::string &filepath
    ):
    myFifo_(toolBox, pipeName),
    myToolBox_(toolBox),
    myFileHandler_(filepath,toolBox),
    pipeName_(pipeName),
    mySigHandler_(SIGPIPE,sigpipe_handler)
{
    myFifo_.createFifo();
}

sendPipeHandler::~sendPipeHandler()
{
    pipeFile_.close();
}

void* TimerThread(void* arg)
{
    ThreadInfo* info = static_cast<ThreadInfo*>(arg);
    int attempt = 0;
    while (attempt++ < info->toolbox->getMaxAttempt()*20) //one loop every 50ms -> *20 to get getMaxAttempt seconds
    {
        info->toolbox->nap(50);
        info->toolbox->updatePrintingElements("Waiting for ipc_receivefile.");
    }
    pthread_cancel(info->thread1);

    return nullptr;
}

void* OpenThread(void* arg)
{
    ThreadInfo* info = static_cast<ThreadInfo*>(arg);
    info->pipeFilePtr->open(info->pipeName,std::ios::out | std::ios::binary);
    pthread_cancel(info->timer);
    return nullptr;
}

void sendPipeHandler::connect()
{
    ThreadInfo info;
    info.pipeName = pipeName_;
    info.pipeFilePtr = &pipeFile_;
    info.toolbox = myToolBox_;
    std::cout << "Connecting to the pipe " << pipeName_ << std::endl;

    ::pthread_create(&info.thread1, nullptr,&OpenThread, (void*)&info);
    ::pthread_create(&info.timer, nullptr,&TimerThread, (void*)&info);
    
    ::pthread_join(info.timer, nullptr);
    std::cout << std::endl;
    void* retval;
    ::pthread_join(info.thread1, &retval); 
    if (retval == PTHREAD_CANCELED)
    {
        throw ipc_exception("Error, can't connect to the other program.\n" );

    }
    if (!pipeFile_.is_open())
    {
        throw std::runtime_error(
            "Error opening the pipe.\n"
        );
    }

    std::cout << "Pipe is opened in both sides." << std::endl;

}

void sendPipeHandler::sendData(void *data, size_t data_size_bytes)
{
    if (!pipeFile_.is_open())
    {
        throw ipc_exception("sendData(). Error, trying to write to a pipe which is not opened.");
    }

    if (sigpipe_received)
    {
        throw ipc_exception("SIGPIPE Received. The receiver may have crashed. \n");
    }

    pipeFile_.write(static_cast<char*>(data), data_size_bytes);

    if (sigpipe_received)
    {
        throw ipc_exception("SIGPIPE Received. The receiver may have crashed. \n");
    }

    //check pipeFile_ state
    auto state = pipeFile_.rdstate();
    if (state == std::ios_base::goodbit)
        return;
    if (state == std::ios_base::failbit && state == std::ios_base::eofbit)
        return;
    if (state == std::ios_base::failbit)
    {
        throw ipc_exception("sendData(). Failbit error. May be set if construction of sentry failed.");
    }
    if (state == std::ios_base::badbit)
    {
        throw ipc_exception("sendData(). Badbit error.");
    }
    throw ipc_exception("sendData(). Unknown error.");
}

size_t sendPipeHandler::transferHeader()
{
    size_t fileSize = myFileHandler_.fileSize();
    Header myHeader(myToolBox_->getKey(), fileSize, myToolBox_);

    sendData(myHeader.getData(), myToolBox_->getDefaultBufferSize());

    return fileSize;
}

size_t sendPipeHandler::transferData(std::vector<char> &buffer)
{
    size_t bufferSize = myToolBox_->getDefaultBufferSize();
    buffer.resize(bufferSize);
    size_t dataRead = myFileHandler_.readFile(buffer.data(), bufferSize);
    sendData(buffer.data(), dataRead);

    return dataRead;
}

#pragma endregion sendPipeHandler

////////////////// receivePipeHandler ////////////
#pragma region receivePipeHandler

receivePipeHandler::receivePipeHandler(
    handyFunctions* toolBox,
    const std::string &pipeName,
    const std::string &filepath
    ):
    myFifo_(toolBox, pipeName),
    myToolBox_(toolBox),
    myFileHandler_(filepath,toolBox),
    pipeName_(pipeName)
{
    int attempt = 0;
    int max_attempt = myToolBox_->getMaxAttempt();
    while(!myToolBox_->checkIfFileExists(pipeName_)
        && attempt++ < max_attempt*20)
    {
        myToolBox_->updatePrintingElements("Waiting for ipc_sendfile.");
        myToolBox_->nap(50);
    }
    if (attempt >= max_attempt)
    {
        throw ipc_exception("Error, can't connect to ipc_sendfile.\n" );
    }
}

receivePipeHandler::~receivePipeHandler()
{
    pipeFile_.close();
    unlink(pipeName_.c_str());
}

void receivePipeHandler::connect()
{
    std::cout << "Connecting to the pipe " << pipeName_ << std::endl;
    pipeFile_.open(pipeName_, std::ios::in | std::ios::binary);
    if (!pipeFile_.is_open())
    {
        throw ipc_exception(
            "Error when trying to connect to the pipe. rdstate:" + pipeFile_.rdstate());
    }
}

size_t receivePipeHandler::transferHeader()
{
    Header myHeader(myToolBox_->getKey(), myToolBox_);

    size_t HeaderSize = receiveData(myHeader.getData(),myToolBox_->getDefaultBufferSize());


    if(HeaderSize < 2*sizeof(size_t) || myHeader.getKey() != myToolBox_->getKey())
    {
        throw ipc_exception("Error, incorrect header found.\n");
    }

    return myHeader.getFileSize();
}
    
size_t receivePipeHandler::transferData(std::vector<char> &buffer) 
{
    size_t bufferSize = myToolBox_->getDefaultBufferSize();
    buffer.resize(bufferSize);
    size_t sizeReadfromPipe = receiveData(buffer.data(), bufferSize);
    myFileHandler_.writeFile(buffer.data(), sizeReadfromPipe);
    return sizeReadfromPipe;
}

size_t receivePipeHandler::receiveData(void* data, size_t bufferSize)
{
    if (!pipeFile_.is_open())
    {
        throw ipc_exception("Error, trying to read in a pipe that is not opened");
    }
    
    pipeFile_.read(static_cast<char*>(data), bufferSize);
    size_t retval = pipeFile_.gcount();

    auto state = pipeFile_.rdstate();
    if (state == std::ios_base::goodbit)
        return retval;
    if (state == std::ios_base::eofbit+std::ios_base::failbit)
    {
        return retval; // end of file
    }
    if (state == std::ios_base::eofbit)
    {
        throw ipc_exception("receiveData(). Eofbit error.");
    }
    if (state == std::ios_base::failbit)
    {
        throw ipc_exception("receiveData(). Failbit error.");
    }
    if (state == std::ios_base::badbit)
    {
        throw ipc_exception("receiveData(). badbit error.");
    }

    return retval;
}



#pragma endregion receivePipeHandler