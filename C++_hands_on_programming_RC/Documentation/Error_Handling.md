# Summary
- [Summary](#summary)
- [Program misuse](#program-misuse)
  - [Incorrect arguments](#incorrect-arguments)
  - [Giving a file to copy that does not exist](#giving-a-file-to-copy-that-does-not-exist)
  - [Giving different protocols between the two programs](#giving-different-protocols-between-the-two-programs)
  - [Killing a program while running](#killing-a-program-while-running)
  - [Case if the IPC channel already exists](#case-if-the-ipc-channel-already-exists)
  - [Case if an argument is given after the protocol name](#case-if-an-argument-is-given-after-the-protocol-name)
  - [Max filename length is reached](#max-filename-length-is-reached)
  - [Other programs are using the protocol with the same name](#other-programs-are-using-the-protocol-with-the-same-name)
- [Hardware-related issues](#hardware-related-issues)
  - [Not enough space in the disk](#not-enough-space-in-the-disk)
  - [Not enough RAM](#not-enough-ram)
  - [Max path length is reached](#max-path-length-is-reached)
  - [Writing file or Reading file become not reachable](#writing-file-or-reading-file-become-not-reachable)

# Program misuse
## Incorrect arguments
If the user gives not enough arguments, too many arguments (like `--pipe --shm`), or unknown arguments the program will end with EXIT_FAILURE and print statements according to the following table.

|Type of incorrect arguments|Example|Statements|
|---|---|---|
|No protocol is provided| `./ipc_receivefile --file myFile` | `No protocol provided. Use --help option to display available commands. Bye!`
|A protocol is provided but the argument `--file` is missing| `./ipc_receivefile --pipe`| `No --file provided. To launch IPCtransfert you need to specify a file which the command --file <nameOfFile>.`|
|Name of the file is missing|`./ipc_receivefile --pipe --file`|`Name of the file is missing. Abord.`|
|Unknown argument|`./ipc_receivefile --message --file myFile`|`Wrong arguments are provided. Use --help to know which ones you can use. Abord.`|
|Too many arguments are provided|`./ipc_receivefile --pipe --file myFile --shm`|`Too many arguments are provided. Abord.`

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

## Case if the IPC channel already exists

## Case if an argument is given after the protocol name

## Max filename length is reached

## Other programs are using the protocol with the same name

# Hardware-related issues
## Not enough space in the disk

## Not enough RAM

## Max path length is reached

## Writing file or Reading file become not reachable

