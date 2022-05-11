#ifndef IPCCOPYFILE_H
#define IPCCOPYFILE_H



#include "Tools.h"
#include "IpcPipe.h"
#include "IpcQueue.h"
#include "IpcShm.h"



enum class protocolList {NONE, QUEUE, PIPE, SHM, HELP, ERROR};
enum class program {SENDER, RECEIVER};



class IpcParameters
{
    public:
        IpcParameters(protocolList protocol, const char* filepath, HandyFunctions* toolBox):protocol_(protocol), filepath_(std::string(filepath)), myToolBox_(toolBox){};
        IpcParameters(int argc, char* const argv[], HandyFunctions* toolBox); 
        protocolList getProtocol() const;
        std::string getFilePath() const;
        std::map<protocolList, std::string> getIpcNames() const;
        std::string correctingIPCName(char* ipcName) const;


    protected:
        protocolList protocol_;
        std::string filepath_ = "";
        HandyFunctions* myToolBox_;
        std::map<protocolList, std::string> IpcNames_ =
        {
            {protocolList::QUEUE, "/QueueIPC"},
            {protocolList::PIPE, "PipeIPC"},
            {protocolList::SHM, "/ShmIPC"}
        };
};


class CopyFileThroughIPC
{
    protected:
        HandyFunctions* myToolBox_;
        IpcParameters myParameters_;
        std::unique_ptr<IpcHandler> myIpcHandler_;
        size_t currentBufferSize_;
        std::vector<char> buffer_;
        program myTypeOfProgram_;
        size_t fileSize;
    public:
        CopyFileThroughIPC(int argc, char* const argv[], HandyFunctions* toolBox, program whichProgram): myToolBox_(toolBox),myParameters_(IpcParameters(argc,argv, toolBox)),myTypeOfProgram_(whichProgram)
        {
            currentBufferSize_ = myToolBox_->getDefaultBufferSize();
        };
        void initSharedPtr();
        int launch();
};



#endif /* IPCCOPYFILE_H */