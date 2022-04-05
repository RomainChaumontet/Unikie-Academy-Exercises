# Summary
- [Summary](#summary)
- [Program misuse](#program-misuse)
  - [Incorrect arguments](#incorrect-arguments)
  - [Giving a file to copy that does not exists](#giving-a-file-to-copy-that-does-not-exists)

# Program misuse
## Incorrect arguments
If the user gives not enough arguments, too many arguments (like `--pipe --shm`), or unknown arguments the program will end with the value 0 and print statements according to the following table.

|Type of incorrect arguments|Example|Statements|
|---|---|---|
|No protocol is provided| `./ipc_receivefile --file myFile` | `No protocol provided. Use --help option to display available commands. Bye!`
|A protocol is provided but the argument `--file` is missing| `./ipc_receivefile --pipe`| `No --file provided. To launch IPCtransfert you need to specify a file which the command --file <nameOfFile>.`|
|Name of the file is missing|`./ipc_receivefile --pipe --file`|`Name of the file is missing. Abort.`|
|Unknown argument|`./ipc_receivefile --message --file myFile`|`Wrong arguments are provided. Use --help to know which ones you can use. Abort.`|
|Too many arguments are provided|`./ipc_receivefile --pipe --file myFile --shm`|`Too many arguments are provided. Abort.`

The test case for this handling error is named `MainTest` and is in [Gtest_manageOpt.cpp](../gtest/Gtest_manageOpt.cpp).

## Giving a file to copy that does not exists
If the user gives a path of a file that does no exist, the program will end with the value 0 and print the statement : `Error, the file specified does not exist. Abord.`.

The test case for this handling error is named `MainTest` and is in [Gtest_manageOpt.cpp](../gtest/Gtest_manageOpt.cpp).
