#ifndef IPCPIPE_H
#define IPCPIPE_H

#include "Tools.h"

volatile extern std::atomic_bool sigpipe_received;

class fifoHandler
{
    private:
        HandyFunctions* myToolBox_;
        std::string pipeName_;
    public:
        fifoHandler(HandyFunctions* toolBox, const std::string &pipeName):myToolBox_(toolBox), pipeName_(pipeName){};
        ~fifoHandler();
        void createFifo() const;
};

class SigHandler
{
    protected:
        struct sigaction sa;
        struct sigaction old_sa;
    public:
    SigHandler(int SIGNUM, void (*sig_handler)(int))
    {
        sigpipe_received = false;
        sa.sa_handler = sig_handler;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        if (sigaction(SIGPIPE, &sa, &old_sa)==-1)
        {
            throw ipc_exception("Error assigning action to signal");
        }
    }
    SigHandler(const SigHandler &) = delete;
    SigHandler &operator=(const SigHandler &) = delete;
    ~SigHandler()
    {
        sigaction(SIGPIPE, &old_sa, NULL);
    }
};

struct ThreadInfo
{
    pthread_t thread1;
    pthread_t timer;
    std::fstream* pipeFilePtr;
    std::string pipeName;
    HandyFunctions* toolbox;
};



class sendPipeHandler : public ipcHandler
{
    private:
        fifoHandler myFifo_;
        HandyFunctions* myToolBox_;
        Reader myFileHandler_;
        std::fstream pipeFile_;
        std::string pipeName_;
        SigHandler mySigHandler_;

    public:
        sendPipeHandler(HandyFunctions* toolBox, const std::string &pipeName, const std::string &filepath);
        virtual ~sendPipeHandler();
        virtual void connect() override;
        virtual void sendData(void *data, size_t data_size_bytes);
        virtual size_t transferHeader() override;
        virtual size_t transferData(std::vector<char> &buffer) override;
};

class receivePipeHandler : public ipcHandler
{
    private:
        fifoHandler myFifo_;
        HandyFunctions* myToolBox_;
        Writer myFileHandler_;
        std::fstream pipeFile_;
        std::string pipeName_;
    public:
        receivePipeHandler(HandyFunctions* toolBox, const std::string &pipeName, const std::string &filepath);
        virtual ~receivePipeHandler();
        virtual size_t receiveData(void* data, size_t bufferSize); //data should have bufferSize space allocated
        virtual void connect() override;
        virtual size_t transferHeader() override;
        virtual size_t transferData(std::vector<char> &buffer) override;
};


#endif // IPCPIPE_H