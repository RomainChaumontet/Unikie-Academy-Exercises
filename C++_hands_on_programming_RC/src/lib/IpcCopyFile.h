#ifndef IPCCOPYFILE_H
#define IPCCOPYFILE_H
#define DEBUG_GETOPT false
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

enum class protocolList {NONE, QUEUE, PIPE, SHM, HELP, TOOMUCHARG, WRONGARG, NOFILE, NOFILEOPT};
inline bool checkIfFileExists (const std::string &filepath);
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
        std::string getName() const;
        std::string changeName(const std::string &name);

        size_t getBufferSize() const;

        virtual void openFile(const std::string &filepath) = 0;
        void closeFile();
        virtual void syncFileWithBuffer() = 0;

        virtual ~copyFilethroughIPC();

    protected:
        std::string name_ = "ipcCopyFile";
        size_t bufferSize_ = 4096;
        std::fstream file_;
        std::vector<char> buffer_;
};

class Writer : virtual public copyFilethroughIPC
{
    public:
        void openFile(const std::string &filepath);
        void syncFileWithBuffer();
};

class Reader : virtual public copyFilethroughIPC
{
    public:
        void openFile(const std::string &filepath);
        void syncFileWithBuffer();
};

#endif /* IPCCOPYFILE_H */