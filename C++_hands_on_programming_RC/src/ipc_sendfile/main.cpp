#include <iostream>
#include "../lib/IpcCopyFile.h"


int main(int argc, char* const argv[])
{
    try
    {
        ipcParameters parameters {argc, argv};
        switch (parameters.getProtocol())
        {
            case protocolList::NONE:
            {
                std::cout << "No protocol provided. Use --help option to display available commands. Bye!" << std::endl;
                return 0;
            }
            case protocolList::TOOMUCHARG:
            {                   
                std::cout << "Too much arguments are provided. Abord." <<std::endl;
                return 0;
            }
            case protocolList::WRONGARG:
            {
                std::cout << "Wrong arguments are provided. Use --help to know which ones you can use. Abord." << std::endl;
                return 0;
            }
            case protocolList::NOFILE:
            {
                std::cout << "No --file provided. To launch IPCtransfert you need to specify a file which the command --file <nameOfFile>." << std::endl;
                return 0;
            }
            case protocolList::NOFILEOPT:
            {
                std::cout << "Name of file is missing. Abord." << std::endl;
                return 0;
            }
            case protocolList::HELP:
            {
                std::cout << "Welcome to this incredible program!" <<std::endl; 
                std::cout << "It can do magic: copy a file in a completely ineffective way." <<std::endl;
                std::cout << "To launch it, you need to provide the IPC protocol and the path of the file." <<std::endl<<std::endl;
                std::cout << "Available protocols are at this time:" <<std::endl;
                std::cout << "      --queue" <<std::endl<<std::endl;
                std::cout << "Examples:" <<std::endl;
                std::cout << "      --queue --file myFile" <<std::endl;
                std::cout << "      --file myFile --queue" <<std::endl;
                return 0;
            }
            case protocolList::QUEUE:
            case protocolList::PIPE:
            case protocolList::SHM:
            {
                std::cout << "Protocol not implemented. Abord"<<std::endl;
                break;
            }
            default:
                break;
        }
    }
    catch (std::exception &e)
    {
        std::cout << "caught :" << e.what() << std::endl;
        throw;
    }
    

    return 0;
}