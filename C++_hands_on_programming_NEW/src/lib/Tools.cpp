#include "Tools.h"


///////////////// handyfunction ///////////////////////////
#pragma region handyFunction

size_t handyFunctions::getDefaultBufferSize() const
{
    return defaultBufferSize;
}

size_t handyFunctions::getKey() const
{
    return key;
}

int handyFunctions::getMaxAttempt() const
{
    return maxAttempt_;
}

void handyFunctions::checkFilePath(const std::string &filepath) const
{
    char currentDir[PATH_MAX];
    if (getcwd(currentDir, PATH_MAX) == NULL)
    {
        throw system_exception("Error. Unable to get the current directory.\n");
    }

    std::string::size_type slashPosition = filepath.rfind('/');
    if (slashPosition == std::string::npos)
    {
        if (filepath.size() > NAME_MAX)
        {
            throw arguments_exception("Error, the name of the file provided is too long.");
        }
    }
    else
    {
        if (filepath.size()-slashPosition > NAME_MAX)
        {
            throw arguments_exception("Error, the name of the file provided is too long.");
        }
        if (slashPosition > PATH_MAX-strlen(currentDir))
        {
            throw arguments_exception("Error, the name of the path provided is too long.");
        }
    }
}

bool handyFunctions::checkIfFileExists (const std::string &filepath) const
{
    struct stat buffer;
    return (stat(filepath.c_str(), &buffer) == 0);
}

size_t handyFunctions::returnFileSize(const std::string &filepath) const
{
    if (checkIfFileExists(filepath) == false)
        throw file_exception("returnFileSize(). File does not exist.");
    struct stat64 buffer;
    stat64(filepath.c_str(), &buffer);
    return buffer.st_size;
}

bool handyFunctions::enoughSpaceAvailable(size_t fileSize) const
{
    struct statvfs systemStat;
    int status = statvfs("/", &systemStat);
    if(status == -1)
    {
        throw system_exception("Error while getting statvfs. Errno:" + std::string(strerror(errno)));
    }
    if(systemStat.f_bavail*systemStat.f_bsize < defaultBufferSize)
        return false;
    if (fileSize < systemStat.f_bavail*systemStat.f_bsize - defaultBufferSize)
        return true;
    else
        return false;
}

void handyFunctions::printInstructions() const
{
    std::cout << "Welcome to this incredible program!" <<std::endl; 
    std::cout << "It can do magic: copy a file in a completely ineffective way." <<std::endl;
    std::cout << "To launch it, you need to provide the IPC protocol and the path of the file." <<std::endl<<std::endl;
    std::cout << "Available protocols are at this time:" <<std::endl;
    std::cout << "      --queue" <<std::endl;
    std::cout << "      --pipe" <<std::endl;
    std::cout << "      --shm" <<std::endl<<std::endl;
    std::cout << "Examples:" <<std::endl;
    std::cout << "      --queue --file myFile" <<std::endl;
    std::cout << "      --file myFile --queue" <<std::endl<<std::endl;
    std::cout << "You can also specify the name of the Ipc channel:" <<std::endl;
    std::cout << "      --pipe pipeChannel --file myFile" <<std::endl;
    std::cout << "      --queue /queueChannel --file myFile" <<std::endl<<std::endl;
    std::cout << "Some restrictions about the Ipc channel:" <<std::endl;
    std::cout << "      * queue and shared memory channel name should start with a /." <<std::endl;
    std::cout << "      * queue and shared memory channel name shall avoid other /." <<std::endl;
    std::cout << "      * ipc channel name should not be more than NAME_MAX character (usually 255)." <<std::endl;
}

void handyFunctions::updatePrintingElements(std::string toPrint, bool forcePrint)
{
    std::chrono::time_point<std::chrono::steady_clock> now =
        std::chrono::steady_clock::now();

    if (std::chrono::duration_cast<std::chrono::milliseconds>(now-last_update_).count()>50 || forcePrint) //do not refresh before 50ms
    {
        //getting the size of the terminal
        struct winsize size;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
        if (size.ws_col<5)
        {
            toPrint.resize(80, ' ');
        }
        else
        {
            toPrint.resize(size.ws_col-2, ' ');
        }

        //updating the screen
        std::cout << '\r' << wheel_[lastChar_] <<  " " << toPrint  << std::flush ;
        lastChar_ = (lastChar_+1) % 4;
        last_update_ = std::chrono::steady_clock::now();
    }
}

void handyFunctions::nap(int timeInMs) const
{
    std::this_thread::sleep_for(std::chrono::milliseconds(timeInMs));
}

void handyFunctions::getTime(struct timespec &ts) const
{
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        throw time_exception("Error getting time");
    }
}

void handyFunctions::printFileSize(size_t fileSize) const
{
    size_t Gb = fileSize/(1024*1024*1024);
    size_t Mb = (fileSize-Gb*1024*1024*1024)/(1024*1024);
    size_t Kb = (fileSize-Gb*1024*1024*1024-Mb*1024*1024)/1024;
    size_t b = (fileSize-Gb*1024*1024*1024-Mb*1024*1024-Kb*1024);

    std::cout << "Transferring a file which size: " << Gb << "GB " << Mb << "MB " << Kb << "KB " << b << "B." << std::endl; 
}

void handyFunctions::checkIf2FilesAreTheSame(const std::string& file1, const std::string& file2) const
{
    char currentDir[PATH_MAX];
    getcwd(currentDir, PATH_MAX);
    std::string completePath1 = "";
    std::string completePath2 = "";

    if (file1[0] != '/')
    {
        completePath1 = std::string(currentDir) + '/';
    }
    if (file2[0] != '/')
    {
        completePath2 = std::string(currentDir) + '/';
    }

    if ((completePath1+file1) == (completePath2+file2))
    {
        throw arguments_exception("Error, the name of the file is the same as the ipc channel name.\n");
    }

}

#pragma endregion handyFunction


/////////////////// fileHandler ///////////////////////////
#pragma region fileHandler

size_t fileHandler::readFile(void* buffer, size_t maxSizeToRead)
{
    size_t dataRead;

    if (!file_.is_open())
    {
        throw file_exception("readFile(). Error. Trying to read a file that is not opened. \n");
    }

    file_.read(static_cast<char*>(buffer), maxSizeToRead);
    dataRead = file_.gcount();

    auto state = file_.rdstate();
    if (state == std::ios_base::goodbit)
        return dataRead;
    if (state == std::ios_base::eofbit+std::ios_base::failbit)
        return dataRead; // end of file
    if (state == std::ios_base::eofbit)
    {
        throw file_exception("readFile(). Eofbit error.");
    }
    if (state == std::ios_base::failbit)
    {
        throw file_exception("readFile(). Failbit error.");
    }
    if (state == std::ios_base::badbit)
    {
        throw file_exception("readFile(). Badbit error.");
    }
    return dataRead;
}

void fileHandler::writeFile(void* buffer, size_t sizeToWrite)
{
    if (!file_.is_open())
    {
        throw file_exception("writeFile(). Error. Trying to write into a file that is not opened. \n");
    }

    file_.write(static_cast<char*>(buffer), sizeToWrite);

    auto state = file_.rdstate();
    if (state == std::ios_base::goodbit)
        return;

    if (state == std::ios_base::failbit)
    {
        throw file_exception("syncFileWithBuffer(). Failbit error. May be set if construction of sentry failed.");
    }
    if (state == std::ios_base::badbit)
    {
        throw file_exception("Writer syncFileWithBuffer(). Badbit error.");
    }
    throw file_exception("Writer syncFileWithBuffer(). Unknown error.");
}

size_t fileHandler::fileSize()
{
    return myToolBox->returnFileSize(filepath_);
}

#pragma endregion fileHandler

/////////////////// Writer ////////////////////////////
#pragma region Writer
Writer::Writer(const std::string&filepath, handyFunctions* toolBox):fileHandler(filepath, toolBox)
{
    if (myToolBox->checkIfFileExists(filepath))
        std::cout << "The file specified to write in already exists. Data will be erased before proceeding."<< std::endl ;

    file_.open(filepath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file_.is_open())
    {
        throw file_exception("Error in std::fstream.open(). rdstate:" + std::to_string(file_.rdstate()));
    }
}

Writer::~Writer()
{
    file_.close();
}

void Writer::cleanInCaseOfThrow()
{
    remove(filepath_.c_str());
}
#pragma endregion Writer

/////////////////// Reader ////////////////////////////
#pragma region Reader
Reader::Reader(const std::string& filepath, handyFunctions* toolBox):fileHandler(filepath,toolBox)
{
    if (!myToolBox->checkIfFileExists(filepath))
    {
        throw file_exception("Error. Trying to open a file for reading which does not exist.");
    }

    if(!myToolBox->enoughSpaceAvailable(myToolBox->returnFileSize(filepath)))
    {
        throw system_exception("Error, not enough space available to copy the file");
    }


    file_.open(filepath, std::ios::in | std::ios::binary);
    if (!file_.is_open())
    {
        throw file_exception("Error in std::fstream.open(). rdstate:" + file_.rdstate());
    }
}

Reader::~Reader()
{
    file_.close();
}
#pragma endregion Readers

////////////////// IpcHandler /////////////////////////
#pragma region IpcHandler

ipcHandler::~ipcHandler()
{}

#pragma endregion IpcHandler


////////////////// Header ////////////
#pragma region Header

Header::Header(size_t key, size_t fileSize, handyFunctions* toolbox):myToolBox_(toolbox),key_(key)
{
    headerVector.emplace_back(key);
    headerVector.emplace_back(fileSize);
    headerVector.resize(myToolBox_->getDefaultBufferSize());
}

Header::Header(size_t key, handyFunctions* toolbox):myToolBox_(toolbox),key_(key)
{
    headerVector.emplace_back(key);
    headerVector.resize(myToolBox_->getDefaultBufferSize());
}

void* Header::getData()
{
    return headerVector.data();
}

size_t Header::getKey()
{
    if(headerVector.size()>1)
        return headerVector[0];
    else 
        throw ipc_exception("Error. The header hasn't the right size.\n");
}

size_t Header::getFileSize()
{
    if(headerVector.size()>2)
        return headerVector[1];
    else
        throw ipc_exception("Error. The header hasn't the right size.\n");
}

#pragma endregion Header