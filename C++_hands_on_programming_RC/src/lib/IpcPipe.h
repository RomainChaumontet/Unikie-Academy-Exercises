#ifndef IPCPIPE_H
#define IPCPIPE_H
#include "IpcCopyFile.h"

class Pipe : public virtual copyFilethroughIPC
{
    protected:
        std::fstream pipeFile_;
    public:
        virtual ~Pipe() = 0;

};


class PipeSendFile : public Pipe, public Reader
{
    public:
        PipeSendFile();
        ~PipeSendFile();
        void syncIPCAndBuffer();

};

class PipeReceiveFile : public Pipe, public Writer
{
    public:
        PipeReceiveFile();
        ~PipeReceiveFile();
        void syncIPCAndBuffer();

};



#endif /* IPCPIPE_H */