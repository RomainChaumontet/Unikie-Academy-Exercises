# Table of contents
- [Table of contents](#table-of-contents)
- [FakeCmdLineOpt](#fakecmdlineopt)
- [CaptureStream](#capturestream)
- [CreateRandomFile](#createrandomfile)
- [Gtest_manageOpt](#gtest_manageopt)
- [Gtest_IpcCopyFile](#gtest_ipccopyfile)

# FakeCmdLineOpt

This class is created just to mock some command line options. This class use some advance C++ elements and can be avoid by creating directly argc/argv but it is more covenient to test the behavior of the program with a large sample group of commands arguments.

To be created, it can take : and iterable object, or directly the command lines options.

The method argv() simulate the argv[] and the method argc() simulate argc.

# CaptureStream

This class is created to catch a stream (std::cerr / std::cout) to test the output of the stream versus what is expected.

# CreateRandomFile

This class is created to generate a random-ish binary file using `dev/urandom`. 

# Gtest_manageOpt

This file aim to implement test cases for the class `ipcParameters` and to test the class `FakeCmdLineOpt` implementation.

# Gtest_IpcCopyFile

This file aim to implement test cases for the class `copyFilethroughIPC` and its children.




