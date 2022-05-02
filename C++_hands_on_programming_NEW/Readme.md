# Table of content
- [Table of content](#table-of-content)
- [Introduction](#introduction)
- [Setup environment, build and test](#setup-environment-build-and-test)
- [How to use the program](#how-to-use-the-program)
- [Design](#design)
- [File structure](#file-structure)
  - [Program sources](#program-sources)
- [Error handling](#error-handling)

# Introduction

These programs have the aim to take a file and copy it. To do it, the program ipc_sendfile read the file and sends the data of the file to the program ipc_receivefile which will write the data to a new file. The exchange is done by IPC.

The IPC methods implemented are:
- [] Queue message passing
- [x] Pipe
- [] Shared memory 

# Setup environment, build and test

To setup your environment, you shall execute `setup_environment.sh`:
```bash
$ ./setup_environment.sh
```

This script will check if Bazel and g++ are already installed and install them if not.

To build the program and get the test report, you can execute the `build_and_run_tests.sh` file:

```bash
$ ./build_and_run_tests.sh
```

The binary and the test report will be in the folder `output`.

# How to use the program
You can use the program with the argument `--help` and command `--pipe --file myFile`. You can specify a name for the IPC channel: `--pipe myPipe --file myFile`.

# Design
![Design](Documentation/Current%20Design.PNG)

# File structure

The program sources are in `/src` folder. The test sources are in the `/gtest` folder.

## Program sources
There are 2 main.cpp files : `src/ipc_receivefile/main.cpp` and `src/ipc_sendfile/main.cpp`.

Those files use the library written in the `/lib` folder.

# Error handling
This program can handle errors specified below:
* Incorrect arguments:
  * No protocol provided - EXIT_FAILURE
  * `--file` missing - EXIT_FAILURE
  * Name of the file is missing - EXIT_FAILURE
  * Unknown argument - EXIT_FAILURE
  * Too many arguments - EXIT_FAILURE
  * Filename or filepath is too long - EXIT_FAILURE
  * IPC channel name is too long - print statement and continue with default IPC Name
  * IPC channel name doesn't start with a `/` for queue and shared memory: continue with adding a `/` at the start of the name
  * Ipc channel contains multiple `/` - print statements and proceeds with default IPC Name
  * File to copy doesn't exist - EXIT_FAILURE
  * Different protocols - EXIT_FAILURE
* Killing a program while running - EXIT_FAILURE for the other program
* IPC Channel already exists - if empty the programs will continue, if there is some message on it EXIT_FAILURE
* Another program uses the IPC channel - EXIT_FAILURE at least for one of the programs
* Not enough space in the disk - EXIT_FAILURE
