#include "IpcCopyFile.h"
#include <getopt.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

bool checkIfFileExists(const std::string &filepath)
{
    struct stat buffer;
    return (stat(filepath.c_str(), &buffer) == 0);
}

size_t returnFileSize(const std::string &filepath)
{
    if (checkIfFileExists(filepath) == 0)
        return 0;
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

ipcParameters::ipcParameters(int argc, char* const argv[])
{
    filepath_ = NULL;
    protocol_ = NONE;

    int opt=0;
    while (protocol_ != HELP)
    {
        int option_index = 0;
        opt = getopt_long (argc, argv, ":",long_options,&option_index);
        if (DEBUG_GETOPT) std::cout << "opt value: " << opt << std::endl;
        if (opt == -1) //no more options
            break;
        switch (opt)
        {
        case 'h':
            protocol_= HELP;
            break;
        case 'q':
            if (protocol_!= NONE)
            {
                protocol_= TOOMUCHARG;
                optind = 0;
                return;
            }
            protocol_= QUEUE;
            break;
        case 'p':
            if (protocol_!= NONE)
            {
                protocol_= TOOMUCHARG;
                optind = 0;
                return;
            }
            protocol_= PIPE;
            break;
        case 's':
            if (DEBUG_GETOPT) std::cout << "Shm choosen" << std::endl;
            if (protocol_!= NONE)
            {
                protocol_= TOOMUCHARG;
                optind = 0;
                return;
            }
            protocol_= SHM;
            break;
        case 'f':
            if (optarg)
                filepath_ = optarg;
            else
            {
                protocol_= NOFILEOPT;
                optind = 0;
                return;
            }
            break;
        case '?':
            protocol_= WRONGARG;
            optind = 0;
            return;
            break;
        case ':':
            protocol_= NOFILEOPT;
            optind = 0;
            return;
            break;
        default:
            break;
        }
    }

    optind = 0;
    if (protocol_!= HELP && !filepath_ && protocol_!= NONE)
    {
        protocol_= NOFILE;
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


std::string copyFilethroughIPC::getName() const
{
    return name_;
}

std::string copyFilethroughIPC::changeName(std::string name)
{
    if (name.size() > 0)
        name_ = name;
    else
        std::cerr << "Error. Trying to change the name of protocol exchange to null. Keep it unchanged." << std::endl;
    return name_;
}

size_t copyFilethroughIPC::getBufferSize() const
{
    return bufferSize_;
}

size_t copyFilethroughIPC::changeBufferSize(size_t bufferSize)
{
    if (bufferSize > 0)
        bufferSize_ = bufferSize;
    else
        std::cerr << "Error. Trying to change the size of the buffer to 0. Keep it unchanged." << std::endl;
    return bufferSize_;
}

bool Reader::openFile(const std::string &filepath)
{
    if (!checkIfFileExists(filepath))
    {
        std::cerr << "Error. Trying to open a file for reading which does not exist."<< std::endl ;
        return false;
    }

    file_.open(filepath, std::ios::in | std::ios::binary);
    if (!file_.is_open())
    {
        std::cerr << "Error. Can't open the file for reading."<< std::endl;
        return false;
    }
    return true;
}

bool Writer::openFile(const std::string &filepath)
{
    if (checkIfFileExists(filepath))
        std::cout << "The file specified to write in already exists. Data will be erased before proceeding."<< std::endl ;
    file_.open(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
    return file_.is_open();
}

copyFilethroughIPC::~copyFilethroughIPC()
{
    if (file_.is_open()) // useless
        file_.close();
}

bool Writer::syncFileWithBuffer()
{
    if (!file_.is_open())
    {
        std::cerr << "Error, trying to write to a file which is not opened." << std::endl;
        return false;
    }
    
    file_.write(buffer_.data(), bufferSize_);

    auto state = file_.rdstate();
    if (state == std::ios_base::goodbit)
        return true;

    if (state == std::ios_base::failbit)
    {
        std::cerr << "Failbit error. May be set if construction of sentry failed." << std::endl;
        return false;
    }
    if (state == std::ios_base::badbit)
    {
        std::cerr 
            << "Badbit error. Either an insertion on the stream failed,"
            << " or some other error happened (such as when this function"
            << " catches an exception thrown by an internal operation). "
            << "When set, the integrity of the stream may have been affected."
            << std::endl;
        return false;
    }
    std::cerr << "Unknown error in writing the file!" << std::endl;
    return false;
}

void Writer::syncFileWithIPC(const std::string &filepath)
{
    openFile(filepath);
    openIPC();

    while (getBufferSize() > 0)
    {
        syncIPCAndBuffer();
        syncFileWithBuffer();
    }
}

bool Reader::syncFileWithBuffer()
{
    if (!file_.is_open())
    {
        std::cerr << "Error, trying to read a file which is not opened." << std::endl;
        return false;
    }

    std::vector<char>(bufferSize_).swap(buffer_);
    file_.read(&buffer_[0],bufferSize_);
    bufferSize_ = file_.gcount();
    buffer_.resize(bufferSize_);

    auto state = file_.rdstate();
    if (state == std::ios_base::goodbit)
        return true;
    if (state == std::ios_base::eofbit+std::ios_base::failbit)
        return true; // end of file
    if (state == std::ios_base::eofbit)
    {
        std::cerr << "Eofbit error. The input sequence has no characters available (as reported by rdbuf()->in_avail() returning -1)."
        << std::endl;
        return false;
    }
    if (state == std::ios_base::failbit)
    {
        std::cerr << "Failbit error. The construction of sentry failed (such as when the stream state was not good before the call)."
        << std::endl;
        return false;
    }
    if (state == std::ios_base::badbit)
    {
        std::cerr 
            << "Error on stream (such as when this function catches an exception thrown by an internal operation)."
            << " When set, the integrity of the stream may have been affected."
            << std::endl;
        return false;
    }
    std::cerr << "Unknown error in writing the file! State:" << state << std::endl;
    return false;
}

void Reader::syncFileWithIPC(const std::string &filepath)
{
    openFile(filepath);
    openIPC();
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

void copyFilethroughIPC::closeFile()
{
    if (file_.is_open())
        file_.close();
}


