#include <iostream>
#include "../lib/IpcCopyFile.h"



int main(int argc, char* const argv[])
{
    try
    {
        handyFunctions myToolBox;
        copyFileThroughIPC IpcWrapper(argc, argv, &myToolBox, program::SENDER);
        IpcWrapper.launch();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}