#ifndef IPCPIPE_H
#define IPCPIPE_H
#include "IpcCopyFile.h"
#include <signal.h>

class Pipe : public virtual copyFilethroughIPC
{
    protected:
        std::fstream pipeFile_;
        std::string name_ = "CopyDataThroughPipe";
    public:
        virtual ~Pipe() = 0;

};


class PipeSendFile : public Pipe, public Reader
{
    struct sigaction sa;

    public:
        PipeSendFile(int maxAttempt, toolBox* myToolBox);
        PipeSendFile(toolBox* myToolBox):PipeSendFile(30, myToolBox){};
        ~PipeSendFile();

        void syncIPCAndBuffer(void *data, size_t &data_size_bytes);
        void syncIPCAndBuffer()
        {
            return syncIPCAndBuffer(buffer_.data(), bufferSize_);
        };
        void sendHeader(const std::string &filepath);

};

class PipeReceiveFile : public Pipe, public Writer
{
    public:
        PipeReceiveFile(int maxAttempt, toolBox* myToolBox);
        PipeReceiveFile(toolBox* myToolBox):PipeReceiveFile(60, myToolBox){};
        ~PipeReceiveFile();
        void syncIPCAndBuffer(void *data, size_t &data_size_bytes);
        void syncIPCAndBuffer()
        {
            syncIPCAndBuffer(buffer_.data(), bufferSize_);
        };

};



#endif /* IPCPIPE_H */