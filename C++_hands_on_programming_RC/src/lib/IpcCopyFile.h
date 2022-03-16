#ifndef IPCCOPYFILE_H
#define IPCCOPYFILE_H
#define DEBUG_GETOPT false
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

enum protocolList {NONE, QUEUE, PIPE, SHM, HELP, TOOMUCHARG, WRONGARG, NOFILE, NOFILEOPT};
inline bool checkIfFileExists (const std::string &filepath);
size_t returnFileSize(const std::string &filepath) ;


class ipcParameters
{
    public:
        ipcParameters(protocolList protocol, char* filepath):protocol_(protocol), filepath_(filepath){};
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
        std::string getName() const;
        std::string changeName(std::string name);

        size_t getBufferSize() const;
        size_t changeBufferSize(size_t bufferSize);

        virtual bool openFile(const std::string &filepath) = 0;
        void closeFile();
        virtual bool syncFileWithBuffer() = 0;

        virtual ~copyFilethroughIPC();

    protected:
        std::string name_ = "ipcCopyFile";
        size_t bufferSize_ = 4096;
        std::fstream file_;
        std::vector<char> buffer_;
};

class Writer : public copyFilethroughIPC
{
    public:
        bool openFile(const std::string &filepath);
        bool syncFileWithBuffer();
};

class Reader : public copyFilethroughIPC
{
    public:
        bool openFile(const std::string &filepath);
        bool syncFileWithBuffer();
};

#endif /* IPCCOPYFILE_H */