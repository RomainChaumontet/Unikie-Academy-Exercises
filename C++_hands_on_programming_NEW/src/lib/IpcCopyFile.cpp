#include "IpcCopyFile.h"



///////////////// ipcParameters //////////////////////////
#pragma region ipcParameters

struct option long_options[]=
{
	  {"help",     no_argument, NULL, 'h'},
	  {"queue",  optional_argument, NULL, 'q'},
	  {"pipe",  optional_argument, NULL, 'p'},
	  {"shm",    optional_argument, NULL, 's'},
	  {"file",  required_argument, NULL, 'f'},
	  {0, 0, 0, 0}
};

ipcParameters::ipcParameters(int argc, char* const argv[], handyFunctions* toolBox):myToolBox_(toolBox)
{
    opterr = 0; //getopt_long won't print errors
    optind = 0; //reset the index of getopt_long
    protocol_ = protocolList::NONE;

    int opt=0;
    while (protocol_ != protocolList::HELP)
    {
        opt = getopt_long (argc, argv, "::",long_options,nullptr);
        if (opt == -1) //no more options
            break;
        switch (opt)
        {
        case 'h':
            protocol_= protocolList::HELP;
            break;
        case 'q':
            if (protocol_!= protocolList::NONE)
            {
                throw arguments_exception("Error, too much arguments are given. Use --help option to know how to use the program.\n\n");
            }
            protocol_= protocolList::QUEUE;

            if (optarg == NULL && optind < argc && argv[optind][0] != '-') //the parameter is after a space
            {
                optarg = argv[optind++];
            }

            if (optarg)
                IpcNames_[protocolList::QUEUE] = correctingIPCName(optarg);
            break;
        case 'p':
            if (protocol_!= protocolList::NONE)
            {
                throw arguments_exception("Error, too much arguments are given. Use --help option to know how to use the program.\n\n");
            }
            protocol_= protocolList::PIPE;

            if (optarg == NULL && optind < argc && argv[optind][0] != '-') //the parameter is after a space
            {
                optarg = argv[optind++];
            }

            if (optarg)
                IpcNames_[protocolList::PIPE] = correctingIPCName(optarg);
            
            break;
        case 's':
            if (protocol_!= protocolList::NONE)
            {
                throw arguments_exception("Error, too much arguments are given. Use --help option to know how to use the program.\n\n");
            }
            protocol_= protocolList::SHM;

            if (optarg == NULL && optind < argc && argv[optind][0] != '-') //the parameter is after a space
            {
                optarg = argv[optind++];
            }

            if (optarg)
                IpcNames_[protocolList::SHM] = correctingIPCName(optarg);
            break;
        case 'f':

            if (optarg == NULL && optind < argc && argv[optind][0] != '-') //the parameter is after a space
            {
                optarg = argv[optind++];
            }

            if (optarg)
            {
                filepath_ = std::string(optarg);
                myToolBox_->checkFilePath(filepath_);
            }
            else
            {
                throw arguments_exception("Error, no file name is given. Use --help option to know how to use the program.\n\n");
            }
            break;
        case '?':
            throw arguments_exception("Error, unknown argument ("+std::string(argv[optind-1])+"). Use --help option to know how to use the program.\n\n");
        default:
            break;
        }
    }

    if (protocol_==protocolList::NONE)
    {
        throw arguments_exception("Error, no protocol provided. Use --help option to know how to use the program.\n");
    }
    if (protocol_!= protocolList::HELP && filepath_ == "" )
    {
        throw arguments_exception("Error, --file is missing.  Use --help option to know how to use the program.\n");
    }
}

protocolList ipcParameters::getProtocol() const
{
    return protocol_;
}

std::string ipcParameters::getFilePath() const
{
    return std::string(filepath_);
}

std::map<protocolList, std::string>  ipcParameters::getIpcNames() const
{
    return IpcNames_;
}

std::string ipcParameters::correctingIPCName(char* ipcName) const
{
    std::string retval(ipcName);
    std::string::size_type slashPosition;

    if(retval.size() > NAME_MAX)
    {
        std::cout << "The name of the ipc channel is too long. Continue with the default name." << std::endl;
        return IpcNames_.at(protocol_);
    }
    
    slashPosition = retval.find('/');

    if (protocol_ == protocolList::QUEUE || protocol_ == protocolList::SHM) //one slash at the start no other slash after
    {
        if (slashPosition == std::string::npos) //no slash in the name
        {
            return '/' + retval;
        }

        slashPosition = retval.find('/', 1); 
        if (slashPosition != std::string::npos) //a slash is inside the name after the first character
        {
            std::cout << "The name of the ipc channel is incorrect. Continue with the default name." << std::endl;
            return IpcNames_.at(protocol_);
        }
    }
    
    ///// Nothing special done for pipes ////
    return retval;
}


#pragma endregion ipcParameters



////////////////// copyFileThroughIPC ////////////
#pragma region copyFileThroughIPC

void copyFileThroughIPC::initSharedPtr()
{
    if (myParameters_.getProtocol() == protocolList::PIPE)
    {
        if (myTypeOfProgram_ == program::SENDER)
            myIpcHandler_ = std::make_shared<sendPipeHandler>(myToolBox_, myParameters_.getIpcNames().at(protocolList::PIPE), myParameters_.getFilePath());
        else
            myIpcHandler_ = std::make_shared<receivePipeHandler>(myToolBox_, myParameters_.getIpcNames().at(protocolList::PIPE), myParameters_.getFilePath());
    }
    else if (myParameters_.getProtocol() == protocolList::QUEUE)
    {
        if (myTypeOfProgram_ == program::SENDER)
            myIpcHandler_ = std::make_shared<sendQueueHandler>(myToolBox_, myParameters_.getIpcNames().at(protocolList::QUEUE), myParameters_.getFilePath());
        else
            myIpcHandler_ = std::make_shared<receiveQueueHandler>(myToolBox_, myParameters_.getIpcNames().at(protocolList::QUEUE), myParameters_.getFilePath());
    }
    else if (myParameters_.getProtocol() == protocolList::SHM)
    {
        if (myTypeOfProgram_ == program::SENDER)
            myIpcHandler_ = std::make_shared<sendShmHandler>(myToolBox_, myParameters_.getIpcNames().at(protocolList::SHM), myParameters_.getFilePath());
        else
            myIpcHandler_ = std::make_shared<receiveShmHandler>(myToolBox_, myParameters_.getIpcNames().at(protocolList::SHM), myParameters_.getFilePath());
    }
    else
    {
        throw arguments_exception("Error, unknown protocols.\n");
    }
}

int copyFileThroughIPC::launch()
{
    if (myParameters_.getProtocol() == protocolList::HELP)
    {
        myToolBox_->printInstructions();
        return EXIT_SUCCESS;
    }

    //check that the filename and the ipcname are not the same
    myToolBox_->checkIf2FilesAreTheSame(myParameters_.getFilePath(), myParameters_.getIpcNames().at(myParameters_.getProtocol()));
    
    initSharedPtr();

    myIpcHandler_->connect();
    fileSize = myIpcHandler_->transferHeader();
    size_t totalDataTransferred = 0;
    size_t lastLoopData = 0;

    if (fileSize == 0)
    {
        throw arguments_exception("Error, the file provided for the copy is empty.\n");
    }
    myToolBox_->printFileSize(fileSize);
    
    do
    {
        lastLoopData = myIpcHandler_->transferData(buffer_);
        totalDataTransferred += lastLoopData;
        myToolBox_->updatePrintingElements("Transferring data - " + std::to_string(totalDataTransferred*100/fileSize) + "%");
    }
    while (lastLoopData == myToolBox_->getDefaultBufferSize() && totalDataTransferred <= fileSize);
    myToolBox_->updatePrintingElements("Transferring data - " + std::to_string(totalDataTransferred*100/fileSize) + "%", true);
    std::cout << std::endl;

    if (fileSize < totalDataTransferred)
    {
        throw ipc_exception("Error. Too much data received. Maybe another program is using the ipc channel. \n");
    }
    if (fileSize > totalDataTransferred)
    {
        throw ipc_exception("Error. Not enough data transferred. Maybe the other program crashed. Or another program received the data.\n");
    }
    std::cout << "All data transferred. Success!" << std::endl;
    return EXIT_SUCCESS;
}

#pragma endregion copyFileThroughIPC

