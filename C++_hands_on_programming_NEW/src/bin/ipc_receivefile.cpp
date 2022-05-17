#include <iostream>
#include "../lib/IpcCopyFile.h"



int main(int argc, char* const argv[])
{

    HandyFunctions myToolBox;
    std::string filePath = "";
    try
    {
        CopyFileThroughIPC IpcWrapper(argc, argv, &myToolBox, program::RECEIVER, filePath);
        IpcWrapper.launch();
    }
    catch (const std::runtime_error &e)
    {
        if (filePath != "")
            remove(filePath.c_str());
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}