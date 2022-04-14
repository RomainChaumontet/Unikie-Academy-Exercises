#ifndef IPCCOPYFILE_H
#define IPCCOPYFILE_H

#define DEBUG_GETOPT false
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

enum class protocolList {NONE, QUEUE, PIPE, SHM, HELP, TOOMUCHARG, WRONGARG, NOFILE, NOFILEOPT};
void checkFilePath(const std::string &filepath);
bool checkIfFileExists (const std::string &filepath);
size_t returnFileSize(const std::string &filepath) ;


class ipcParameters
{
    public:
        ipcParameters(protocolList protocol, const char* filepath):protocol_(protocol), filepath_(filepath){};
        ipcParameters(int argc, char* const argv[]); 

        protocolList getProtocol() const;
        const char* getFilePath() const;
        void launch();

    private:
        protocolList protocol_;
        const char* filepath_;
};

class copyFilethroughIPC
{
    public:
        size_t getBufferSize() const;

        virtual void openFile(const std::string &filepath) = 0;
        void closeFile();
        virtual void syncFileWithBuffer() = 0;
        virtual void syncIPCAndBuffer(void *data, size_t &data_size_bytes) = 0;
        virtual void syncIPCAndBuffer() =0;
        virtual void syncFileWithIPC(const std::string &filepath) = 0;
        size_t getDefaultBufferSize();

        virtual ~copyFilethroughIPC();
        void sendHeader(const std::string &filepath);
        void receiveHeader();

    protected:
        size_t defaultBufferSize_ = 4096;
        size_t bufferSize_ = defaultBufferSize_;
        size_t fileSize_;
        std::fstream file_;
        std::vector<char> buffer_;
        bool continueGettingData_ = true;
        int maxAttempt_;
        std::string endingSentence_ = "All data is sent. You can leave.";
        std::vector<char> endingVector_ = std::vector<char>(endingSentence_.begin(),endingSentence_.end());
};

class Writer : virtual public copyFilethroughIPC
{
    public:
        void openFile(const std::string &filepath);
        void syncFileWithBuffer();
        virtual void syncFileWithIPC(const std::string &filepath);
};

class Reader : virtual public copyFilethroughIPC
{
    public:
        void openFile(const std::string &filepath);
        void syncFileWithBuffer();
        virtual void syncFileWithIPC(const std::string &filepath);
        virtual void waitForReceiverTerminate(){};
};


int receiverMain(int argc, char* const argv[]);
int senderMain(int argc, char* const argv[]);

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


class Header
{
    std::vector<size_t> main_;
    size_t start = 156886431;

    public:
        Header(const std::string &filename, int defaultsize)
        {
            main_.emplace_back(start);
            main_.emplace_back(returnFileSize(filename));
            main_.resize(defaultsize);
        };
        Header(int defaultsize)
        {
            main_.emplace_back(start);
            main_.resize(defaultsize);
        };
        std::vector<size_t> &getHeader()
        {
            return main_;
        };
        const size_t sizeFile() const
        {
            return main_[1];
        };
};
#endif /* IPCCOPYFILE_H */