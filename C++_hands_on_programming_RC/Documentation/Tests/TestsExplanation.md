# Table of contents
- [Table of contents](#table-of-contents)
- [FakeCmdLineOpt](#fakecmdlineopt)
- [CaptureStream](#capturestream)
- [CreateRandomFile](#createrandomfile)
- [Gtest_manageOpt](#gtest_manageopt)
- [Gtest_IpcCopyFile](#gtest_ipccopyfile)
- [Gtest_IpcQueue](#gtest_ipcqueue)
- [Gtest_IpcShm](#gtest_ipcshm)

# FakeCmdLineOpt

This class is created just to mock some command-line options. This class uses some advanced C++ elements and can be avoided by creating directly argc/argv but it is more convenient to test the behavior of the program with a large sample group of commands arguments.

To be created, it can take: an iterable object, or directly the command lines options.

The method argv() simulate the argv[] and the method argc() simulate argc.

# CaptureStream

This class is created to catch a stream (std::cerr / std::cout) to test the output of the stream versus what is expected.

# CreateRandomFile

This class is created to generate a random-ish binary file using `dev/urandom`. 

# Gtest_manageOpt

This file aims to implement test cases for the class `ipcParameters` and to test the class `FakeCmdLineOpt` implementation.

# Gtest_IpcCopyFile

This file aims to implement test cases for the class `copyFilethroughIPC`, `Writer`, `Reader` defined in `IpcCopyfile.h` and `IpcCopyfile.cpp`.

The only exception is the function `syncFileWithIPC` which is tested depending on the IPC method.

# Gtest_IpcQueue

This file aims to implement tests cases for the class defined in `IpcQueue.h` and `IpcQueue.cpp`.

# Gtest_IpcShm

This file aims to implement tests cases for the class defined in `IpcShm.h` and `IpcShm.cpp`.






