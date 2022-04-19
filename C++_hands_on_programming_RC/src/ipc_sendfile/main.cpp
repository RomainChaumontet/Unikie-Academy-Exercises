#include <iostream>
#include "../lib/IpcCopyFile.h"


int main(int argc, char* const argv[])
{
    toolBox myToolBox;
    ipcRun IpcWrapper{&myToolBox};
    return IpcWrapper.senderMain(argc, argv);
}