# Summary
- [Summary](#summary)
- [Program misuse](#program-misuse)
  - [Incorrect arguments](#incorrect-arguments)
  - [Giving a file to copy that does not exist](#giving-a-file-to-copy-that-does-not-exist)
  - [Giving different protocols between the two programs](#giving-different-protocols-between-the-two-programs)
  - [Killing a program while running](#killing-a-program-while-running)
  - [Case if the IPC channel already exists / Another program uses the IPC channel](#case-if-the-ipc-channel-already-exists--another-program-uses-the-ipc-channel)
  - [Case if an argument is given after the protocol name](#case-if-an-argument-is-given-after-the-protocol-name)
  - [Max filename length is reached](#max-filename-length-is-reached)
- [Hardware-related issues](#hardware-related-issues)
  - [Not enough space in the disk](#not-enough-space-in-the-disk)
  - [Not enough RAM](#not-enough-ram)
  - [Max path length is reached](#max-path-length-is-reached)
  - [Writing file or Reading file becomes not reachable](#writing-file-or-reading-file-becomes-not-reachable)

# Program misuse
## Incorrect arguments
If the user gives not enough arguments, too many arguments (like `--pipe --shm`), or unknown arguments the program will end with EXIT_FAILURE and print statements according to the following table.

|Type of incorrect arguments|Example|Statements|
|---|---|---|
|No protocol is provided| `./ipc_receivefile --file myFile` | `No protocol provided. Use --help option to display available commands. Bye!`
|A protocol is provided but the argument `--file` is missing| `./ipc_receivefile --pipe`| `No --file provided. To launch IPCtransfert you need to specify a file which the command --file <nameOfFile>.`|
|Name of the file is missing|`./ipc_receivefile --pipe --file`|`Name of the file is missing. Abort.`|
|Unknown argument|`./ipc_receivefile --message --file myFile`|`Wrong arguments are provided. Use --help to know which ones you can use. Abort.`|
|Too many arguments are provided|`./ipc_receivefile --pipe --file myFile --shm`|`Too many arguments are provided. Abort.`

The test case for this handling error is named `MainTest` and is in [Gtest_manageOpt.cpp](../gtest/Gtest_manageOpt.cpp).

## Giving a file to copy that does not exist
If the user gives a path of a file that does not exist, the program will end with the value EXIT_FAILURE and print the statement: `Error, the file specified does not exist. Abord.`.

The test case for this handling error is named `MainTest` and is in [Gtest_manageOpt.cpp](../gtest/Gtest_manageOpt.cpp).

## Giving different protocols between the two programs
If the user doesn't give the same protocol to the two programs or he launches only one. They will try to connect between them for 30 seconds. After, the programs will end with EXIT_FAILURE  and print the statement: `Error, can't connect to the other program.`.

The test case for this handling error is named `NoOtherProgram` and is in the Gtest file corresponding to each protocol.

## Killing a program while running
If the user or the system kills one program while the exchange of data is runnning. The program will end with EXIT_FAILURE after 30 seconds with the statement: `Error. Can't find the other program. Did it crash ?`.

The test case for this handling error is named `KillingAProgram` and is in the Gtest file corresponding to each protocol.

## Case if the IPC channel already exists / Another program uses the IPC channel
The receiver is supposed to receive a header, in case the first message received is not a header the ipc_receivefile program will end with EXIT_FAILURE and print `Error. Another message is present. Maybe another program uses this IPC.`.

This header gives the receiver the size of the file, and at the end of the transfer, the receiver will compare this size with the actual size of the copy, if there is a mismatch, the receiver will end with EXIT_FAILURE and print `Error, filesize mismatch. Maybe another program uses the IPC.`.

Special feature for queues:
If a queue is already opened and has some messages on it:
* ipc_sendfile will throw with the statement: ` Error. A queue with some messages already exists.\n"`
* ipc_receivefile will throw only because ipc_sendfile won't connect.

The test suite for this error is named `IPCUsedByAnotherProgram`.
## Case if an argument is given after the protocol name
*Todo*
## Max filename length is reached
If the length of the path (or the name of the file) exceeds PATH_MAX (or NAME_MAX). The program will end with EXIT_FAILURE and print:
* if the path is too long: `Error. The path of the file is too long.`.
* if the name is too long: `Error. The name of the file is too long.`.

The test case for this error handling is `FileNameOrPathTooLong`.
# Hardware-related issues
## Not enough space in the disk
The ipc_sendfile program checks if there is enough space on the disk to copy the file, with the function `enoughSpaceAvailable`. If the disk has not enough space, the program will end with EXIT_FAILURE and print: `Error, not enough space on the disk to copy the file.`.

## Not enough RAM
*Todo*
## Max path length is reached
*Todo*
## Writing file or Reading file becomes not reachable
*Todo*
