#ifndef TOOLS_H
#define TOOLS_H


#include <string>
#include <string.h>
#include <map>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <fstream>
#include <time.h>
#include <signal.h>
#include <memory>
#include <sys/stat.h>
#include <chrono>
#include <algorithm>
#include <thread>
#include <limits.h>
#include <sys/statvfs.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <atomic>
#include <mqueue.h>
#include <future>
#include <semaphore.h>
#include <sys/mman.h>
#include <cstring>

struct SemName
{
    std::string senderSemaphoreName;
    std::string receiverSemaphoreName;
};

class HandyFunctions // Wrap all free functions and global variables
{
    protected:
        size_t key = 151563468; // a randomish number to be used as a key
        size_t defaultBufferSize = 4096;
        int maxAttempt_ = 30;

        //print elements
        size_t lastChar_ = 0;
        std::string wheel_ = "|/-\\";
        std::chrono::time_point<std::chrono::steady_clock> last_update_ = std::chrono::steady_clock::now() - std::chrono::milliseconds(60); //the - std::chrono::milliseconds(60) is to be sure that the first print will work


    public:
        virtual ~HandyFunctions(){};

        virtual size_t getDefaultBufferSize() const;
        virtual size_t getKey() const;
        virtual int getMaxAttempt() const;


        virtual void checkFilePath(const std::string &filepath) const;
        virtual bool checkIfFileExists (const std::string &filepath) const;
        virtual size_t returnFileSize(const std::string &filepath) const;
        virtual bool enoughSpaceAvailable(const size_t fileSize) const;
        virtual void printInstructions() const;
        virtual void updatePrintingElements(std::string info, bool forcePrint = false);
        virtual void nap(int timeInMs) const; //time in milliseconds
        virtual void getTime(struct timespec &ts) const;
        virtual void printFileSize(size_t fileSize) const;
        virtual void compareFileNames(const std::string& file1, const std::string& file2) const;
        virtual SemName getSemName(const std::string& IpcName) const;
};


class file_exception : public std::runtime_error
{
    public:
        using std::runtime_error::runtime_error;
};

class ipc_exception : public std::runtime_error
{
    public:
        using std::runtime_error::runtime_error;
};

class arguments_exception : public std::runtime_error
{
    public:
        using std::runtime_error::runtime_error;
};

class system_exception : public std::runtime_error
{
    public:
        using std::runtime_error::runtime_error;
};
class time_exception : public std::runtime_error
{
    public:
        using std::runtime_error::runtime_error;
};


class FileHandler
{
    protected:
        std::fstream file_;
        std::string filepath_;
        HandyFunctions* myToolBox;
    public:
        FileHandler(const std::string&filepath, HandyFunctions* toolBox):filepath_(filepath),myToolBox(toolBox){};
        virtual ~FileHandler(){};
        size_t readFile(void* buffer, size_t maxSizeToRead); //buffer is supposed to point into a space allocated for at least maxSizeToRead bytes.
        void writeFile(void* buffer, size_t sizeToWrite); //the data pointed by the buffer are supposed to be at least sizeToWrite bytes
        size_t fileSize();
};

class Writer : public FileHandler
{
    public:
        Writer(const std::string&filepath, HandyFunctions* toolBox); //open file
        ~Writer(); //close file
        void cleanInCaseOfThrow();
};

class Reader : public FileHandler
{
    public:
        Reader(const std::string&filepath, HandyFunctions* toolBox); //open file
        ~Reader(); //close file
};


class IpcHandler
{
    public:
        IpcHandler(){};
        virtual void connect()=0;
        virtual size_t transferHeader()=0;
        virtual size_t transferData(std::vector<char> &buffer)=0;
        virtual ~IpcHandler()=0;
};

class Header
{
    protected:
        std::vector<size_t> headerVector;
        HandyFunctions* myToolBox_;
        size_t key_;
    public:
        Header(size_t key, size_t fileSize, HandyFunctions* toolbox);
        Header(size_t key, HandyFunctions* toolbox);
        void* getData();
        size_t getKey();
        size_t getFileSize();
};

#endif  //TOOLS_H