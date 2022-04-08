#ifndef IPCPIPE_H
#define IPCPIPE_H
#include "IpcCopyFile.h"

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
    public:
        PipeSendFile(int maxAttempt);
        PipeSendFile():PipeSendFile(30){};
        ~PipeSendFile();
        void syncIPCAndBuffer();

};

class PipeReceiveFile : public Pipe, public Writer
{
    public:
        PipeReceiveFile(int maxAttempt);
        PipeReceiveFile():PipeReceiveFile(60){};
        ~PipeReceiveFile();
        void syncIPCAndBuffer();

};



#endif /* IPCPIPE_H */