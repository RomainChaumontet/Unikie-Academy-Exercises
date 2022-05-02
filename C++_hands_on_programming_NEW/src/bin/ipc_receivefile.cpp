#include <iostream>
#include "../lib/IpcCopyFile.h"



int main(int argc, char* const argv[])
{

    handyFunctions myToolBox;
    try
    {
        copyFileThroughIPC IpcWrapper(argc, argv, &myToolBox, program::RECEIVER);
        IpcWrapper.launch();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}