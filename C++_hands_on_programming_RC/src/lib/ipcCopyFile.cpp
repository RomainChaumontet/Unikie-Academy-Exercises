#include "IpcCopyFile.h"
#include <getopt.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include "../lib/IpcQueue.h"
#include "../lib/IpcPipe.h"
#include "../lib/IpcShm.h"

bool checkIfFileExists(const std::string &filepath)
{
    struct stat buffer;
    return (stat(filepath.c_str(), &buffer) == 0);
}

size_t returnFileSize(const std::string &filepath)
{
    if (checkIfFileExists(filepath) == 0)
        throw std::runtime_error("returnFileSize(). File does not exist.");
    struct stat buffer;
    stat(filepath.c_str(), &buffer);
    return buffer.st_size;
}

struct option long_options[]=
{
	  {"help",     no_argument, NULL, 'h'},
	  {"queue",  no_argument, NULL, 'q'},
	  {"pipe",  no_argument, NULL, 'p'},
	  {"shm",    no_argument, NULL, 's'},
	  {"file",  required_argument, NULL, 'f'},
	  {0, 0, 0, 0}
};

////////////// ipcParameters class ///////////////////////
ipcParameters::ipcParameters(int argc, char* const argv[])
{
    filepath_ = NULL;
    protocol_ = protocolList::NONE;

    int opt=0;
    while (protocol_ != protocolList::HELP)
    {
        int option_index = 0;
        opt = getopt_long (argc, argv, ":",long_options,&option_index);
        if (DEBUG_GETOPT) std::cout << "opt value: " << opt << std::endl;
        if (opt == -1) //no more options
            break;
        switch (opt)
        {
        case 'h':
            protocol_= protocolList::HELP;
            break;
        case 'q':
            if (protocol_!= protocolList::NONE)
            {
                protocol_= protocolList::TOOMUCHARG;
                optind = 0;
                return;
            }
            protocol_= protocolList::QUEUE;
            break;
        case 'p':
            if (protocol_!= protocolList::NONE)
            {
                protocol_= protocolList::TOOMUCHARG;
                optind = 0;
                return;
            }
            protocol_= protocolList::PIPE;
            break;
        case 's':
            if (DEBUG_GETOPT) std::cout << "Shm choosen" << std::endl;
            if (protocol_!= protocolList::NONE)
            {
                protocol_= protocolList::TOOMUCHARG;
                optind = 0;
                return;
            }
            protocol_= protocolList::SHM;
            break;
        case 'f':
            if (optarg)
                filepath_ = optarg;

            else
            {
                protocol_= protocolList::NOFILEOPT;
                optind = 0;
                return;
            }
            break;
        case '?':
            protocol_= protocolList::WRONGARG;
            optind = 0;
            return;
            break;
        case ':':
            protocol_= protocolList::NOFILEOPT;
            optind = 0;
            return;
            break;
        default:
            break;
        }
    }

    optind = 0;
    if (protocol_!= protocolList::HELP && !filepath_ && protocol_!= protocolList::NONE)
    {
        protocol_= protocolList::NOFILE;
    }
}

protocolList ipcParameters::getProtocol() const
{
    return protocol_;
}

const char* ipcParameters::getFilePath() const
{
    return filepath_;
}

////////////// copyFilethroughIPC class ///////////////////////

size_t copyFilethroughIPC::getBufferSize() const
{
    return bufferSize_;
}

copyFilethroughIPC::~copyFilethroughIPC()
{
    if (file_.is_open()) // useless
        file_.close();
}

void copyFilethroughIPC::closeFile()
{
    if (file_.is_open())
        file_.close();
}

////////////// Writer class ///////////////////////
void Writer::openFile(const std::string &filepath)
{
    if (checkIfFileExists(filepath))
        std::cout << "The file specified to write in already exists. Data will be erased before proceeding."<< std::endl ;

    file_.open(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file_.is_open())
    {
        throw std::runtime_error("Error in std::fstream.open(). rdstate:" + file_.rdstate());
    }
}

void Writer::syncFileWithBuffer()
{
    if (!file_.is_open())
    {
        throw std::runtime_error("syncFileWithBuffer(). Error, trying to write to a file which is not opened.");
    }
    
    file_.write(buffer_.data(), bufferSize_);

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

void Writer::syncFileWithIPC(const std::string &filepath)
{
    openFile(filepath);

    while (bufferSize_ > 0)
    {
        syncIPCAndBuffer();
        syncFileWithBuffer();
    }
}


/////////////////// Reader Class
void Reader::openFile(const std::string &filepath)
{
    if (!checkIfFileExists(filepath))
    {
        throw std::runtime_error("Error. Trying to open a file for reading which does not exist.");
    }

    file_.open(filepath, std::ios::in | std::ios::binary);
    if (!file_.is_open())
    {
        throw std::runtime_error("Error in std::fstream.open(). rdstate:" + file_.rdstate());
    }
}

void Reader::syncFileWithBuffer()
{
    if (!file_.is_open())
    {
        throw std::runtime_error("syncFileWithBuffer(). Error, trying to read a file which is not opened.");
    }

    std::vector<char>(bufferSize_).swap(buffer_);
    file_.read(buffer_.data(),bufferSize_);
    bufferSize_ = file_.gcount();
    buffer_.resize(bufferSize_);

    auto state = file_.rdstate();
    if (state == std::ios_base::goodbit)
        return;
    if (state == std::ios_base::eofbit+std::ios_base::failbit)
        return; // end of file
    if (state == std::ios_base::eofbit)
    {
        throw std::runtime_error("syncFileWithBuffer(). Eofbit error.");
        return;
    }
    if (state == std::ios_base::failbit)
    {
        throw std::runtime_error("syncFileWithBuffer(). Failbit error.");
        return;
    }
    if (state == std::ios_base::badbit)
    {
        throw std::runtime_error("Reader syncFileWithBuffer(). badbit error.");
        return;
    }
}

void Reader::syncFileWithIPC(const std::string &filepath)
{
    openFile(filepath);
    ssize_t fileSize = returnFileSize(filepath);
    ssize_t datasent = 0;


    while (datasent < fileSize)
    {
        syncFileWithBuffer();
        syncIPCAndBuffer();
        datasent += getBufferSize();
    }

    //send and empty message to tell that's all.
    syncFileWithBuffer();
    syncIPCAndBuffer();
}




int receiverMain(int argc, char* const argv[])
{
    try
    {
        ipcParameters parameters {argc, argv};
        switch (parameters.getProtocol())
        {
            case protocolList::NONE:
            {
                std::cout << "No protocol provided. Use --help option to display available commands. Bye!" << std::endl;
                return 0;
            }
            case protocolList::TOOMUCHARG:
            {                   
                std::cout << "Too many arguments are provided. Abort." <<std::endl;
                return 0;
            }
            case protocolList::WRONGARG:
            {
                std::cout << "Wrong arguments are provided. Use --help to know which ones you can use. Abort." << std::endl;
                return 0;
            }
            case protocolList::NOFILE:
            {
                std::cout << "No --file provided. To launch IPCtransfert you need to specify a file which the command --file <nameOfFile>." << std::endl;
                return 0;
            }
            case protocolList::NOFILEOPT:
            {
                std::cout << "Name of the file is missing. Abort." << std::endl;
                return 0;
            }
            case protocolList::HELP:
            {
                std::cout << "Welcome to this incredible program!" <<std::endl; 
                std::cout << "It can do magic: copy a file in a completely ineffective way." <<std::endl;
                std::cout << "To launch it, you need to provide the IPC protocol and the path of the file." <<std::endl<<std::endl;
                std::cout << "Available protocols are at this time:" <<std::endl;
                std::cout << "      --queue" <<std::endl<<std::endl;
                std::cout << "      --pipe" <<std::endl<<std::endl;
                std::cout << "Examples:" <<std::endl;
                std::cout << "      --queue --file myFile" <<std::endl;
                std::cout << "      --file myFile --queue" <<std::endl;
                return 0;
            }
            case protocolList::QUEUE:
            {
        
                QueueReceiveFile myReceiveFile;
                myReceiveFile.syncFileWithIPC(parameters.getFilePath());
                break;
            }
            case protocolList::PIPE:
            {
                
                PipeReceiveFile myReceiveFile;
                myReceiveFile.syncFileWithIPC(parameters.getFilePath());
                break;
            }
            case protocolList::SHM:
            {
                ShmReceiveFile myReceiveFile;
                myReceiveFile.syncFileWithIPC(parameters.getFilePath());
                break;
            }
            default:
                break;
        }
    }
    catch (std::exception &e)
    {
        std::cout << "caught :" << e.what() << std::endl;
        return 0;
    }
    
    return 1;
}


int senderMain(int argc, char* const argv[])
{
    try
    {
        ipcParameters parameters {argc, argv};
        if (
            (
                parameters.getProtocol() == protocolList::SHM
                || parameters.getProtocol() == protocolList::QUEUE
                || parameters.getProtocol() == protocolList::PIPE
            )
            && !checkIfFileExists(parameters.getFilePath()))
        {
            std::cout << "Error, the file specified does not exist. Abord." << std::endl;
            return 0;
        }
        switch (parameters.getProtocol())
        {
            case protocolList::NONE:
            {
                std::cout << "No protocol provided. Use --help option to display available commands. Bye!" << std::endl;
                return 0;
            }
            case protocolList::TOOMUCHARG:
            {                   
                std::cout << "Too many arguments are provided. Abort." <<std::endl;
                return 0;
            }
            case protocolList::WRONGARG:
            {
                std::cout << "Wrong arguments are provided. Use --help to know which ones you can use. Abort." << std::endl;
                return 0;
            }
            case protocolList::NOFILE:
            {
                std::cout << "No --file provided. To launch IPCtransfert you need to specify a file which the command --file <nameOfFile>." << std::endl;
                return 0;
            }
            case protocolList::NOFILEOPT:
            {
                std::cout << "Name of the file is missing. Abort." << std::endl;
                return 0;
            }
            case protocolList::HELP:
            {
                std::cout << "Welcome to this incredible program!" <<std::endl; 
                std::cout << "It can do magic: copy a file in a completely ineffective way." <<std::endl;
                std::cout << "To launch it, you need to provide the IPC protocol and the path of the file." <<std::endl<<std::endl;
                std::cout << "Available protocols are at this time:" <<std::endl;
                std::cout << "      --queue" <<std::endl<<std::endl;
                std::cout << "Examples:" <<std::endl;
                std::cout << "      --queue --file myFile" <<std::endl;
                std::cout << "      --file myFile --queue" <<std::endl;
                return 0;
            }
            case protocolList::QUEUE:
            {
                QueueSendFile mySendFile;
                mySendFile.syncFileWithIPC(parameters.getFilePath());
                break;
            }
            case protocolList::PIPE:
            {
                PipeSendFile mySendFile;
                mySendFile.syncFileWithIPC(parameters.getFilePath());
                break;
            }
            case protocolList::SHM:
            {
                ShmSendFile mySendFile;
                mySendFile.syncFileWithIPC(parameters.getFilePath());
                break;
            }
            default:
                break;
        }
    }
    catch (std::exception &e)
    {
        std::cout << "caught :" << e.what() << std::endl;
        return 0;
    }
    

    return 1;
}