#ifndef IPCCOPYFILE_H
#define IPCCOPYFILE_H



#include "Tools.h"
#include "IpcPipe.h"



enum class protocolList {NONE, QUEUE, PIPE, SHM, HELP, ERROR};
enum class program {SENDER, RECEIVER};



class ipcParameters
{
    public:
        ipcParameters(protocolList protocol, const char* filepath, handyFunctions* toolBox):protocol_(protocol), filepath_(std::string(filepath)), myToolBox_(toolBox){};
        ipcParameters(int argc, char* const argv[], handyFunctions* toolBox); 
        protocolList getProtocol() const;
        std::string getFilePath() const;
        std::map<protocolList, std::string> getIpcNames() const;
        std::string correctingIPCName(char* ipcName) const;


    protected:
        protocolList protocol_;
        std::string filepath_ = "";
        handyFunctions* myToolBox_;
        std::map<protocolList, std::string> IpcNames_ =
        {
            {protocolList::QUEUE, "/QueueIPC"},
            {protocolList::PIPE, "PipeIPC"},
            {protocolList::SHM, "/ShmIPC"}
        };
};



class sendShmHandler : public ipcHandler
{

};

class receiveShmHandler : public ipcHandler
{

};

class sendQueueHandler : public ipcHandler
{

};

class receiveQueueHandler : public ipcHandler
{

};

class copyFileThroughIPC
{
    protected:
        handyFunctions* myToolBox_;
        ipcParameters myParameters_;
        std::shared_ptr<ipcHandler> myIpcHandler_;
        size_t currentBufferSize_;
        std::vector<char> buffer_;
        program myTypeOfProgram_;
        size_t fileSize;
    public:
        copyFileThroughIPC(int argc, char* const argv[], handyFunctions* toolBox, program whichProgram): myToolBox_(toolBox),myParameters_(ipcParameters(argc,argv, toolBox)),myTypeOfProgram_(whichProgram)
        {
            currentBufferSize_ = myToolBox_->getDefaultBufferSize();
        };
        void initSharedPtr();
        int launch();
};



#endif /* IPCCOPYFILE_H */