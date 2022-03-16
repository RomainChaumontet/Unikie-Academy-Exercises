#include <gtest/gtest.h>
#include "../src/lib/IpcCopyFile.h"
#include "Gtest_Ipc.h"
#include <deque>
#include <vector>
#include <map>

std::map<std::string, inputLineOpt> inputMap =
{
  {"OK_Help",
        inputLineOpt {
          HELP,
          NULL,
          std::vector<const char*> {"--help"}
          }},
  {"OK_Queue_File_NameOfFile",
        inputLineOpt {
          QUEUE,
          "nameOfFile",
          std::vector<const char*> {"--queue", "--file","nameOfFile"}
          }},
  {"OK_Pipe_File_NameOfFile",
      inputLineOpt {
        PIPE,
        "nameOfFile",
        std::vector<const char*> {"--pipe", "--file","nameOfFile"}
        }},
  {"OK_Shm_File_NameOfFile",
        inputLineOpt {
          SHM,
          "nameOfFile",
          std::vector<const char*> {"--shm", "--file","nameOfFile"}
          }},
  {"OK_File_NameOfFile_Queue",
        inputLineOpt {
          QUEUE,
          "nameOfFile",
          std::vector<const char*> {"--file", "nameOfFile","--queue"}
          }},
  {"OK_File_NameOfFile_Pipe",
        inputLineOpt {
          PIPE,
          "nameOfFile",
          std::vector<const char*> {"--file", "nameOfFile","--pipe"}
          }},
  {"OK_File_NameOfFile_Shm",
        inputLineOpt {
          SHM,
          "nameOfFile",
          std::vector<const char*> {"--file", "nameOfFile","--shm"}
          }},
  {"NOK_",
        inputLineOpt {
          NONE,
          NULL,
          std::vector<const char*> {""}
          }},
  {"NOK_hqasjbta",
        inputLineOpt {
          WRONGARG,
          NULL,
          std::vector<const char*> {"-hqasjbta"}
          }},
  {"NOK_randomArgument",
        inputLineOpt {
          WRONGARG,
          NULL,
          std::vector<const char*> {"--randomArgument"}
          }},
  {"NOK_queue",
        inputLineOpt {
          NOFILE,
          NULL,
          std::vector<const char*> {"--queue"}
          }},
  {"NOK_shm",
        inputLineOpt {
          NOFILE,
          NULL,
          std::vector<const char*> {"--shm"}
          }},
  {"NOK_pipe",
        inputLineOpt {
          NOFILE,
          NULL,
          std::vector<const char*> {"--pipe"}
          }},
  {"NOK_file",
        inputLineOpt {
          NOFILEOPT,
          NULL,
          std::vector<const char*> {"--file"}
          }},
  {"NOK_file_nameOfFile",
        inputLineOpt {
          NONE,
          "nameOfFile.extension",
          std::vector<const char*> {"--file", "nameOfFile.extension"}
          }},
  {"NOK_queue_file",
        inputLineOpt {
          NOFILEOPT,
          NULL,
          std::vector<const char*> {"--queue","--file"}
          }},
  {"NOK_pipe_file",
        inputLineOpt {
          NOFILEOPT,
          NULL,
          std::vector<const char*> {"--pipe","--file"}
          }},
  {"NOK_shm_file",
        inputLineOpt {
          NOFILEOPT,
          NULL,
          std::vector<const char*> {"--shm","--file"}
          }},
  {"NOK_pipe_file_nameOfFile_shm",
        inputLineOpt {
          TOOMUCHARG,
          "nameOfFile",
          std::vector<const char*> {"--pipe","--file","nameOfFile","--shm"}
          }},
  {"NOK_queue_file_nameOfFile_pipe",
        inputLineOpt {
          TOOMUCHARG,
          "nameOfFile",
          std::vector<const char*> {"--queue","--file","nameOfFile","--pipe"}
          }}
};


class FakeCmdLineOptTest : public ::testing::TestWithParam<std::pair<const std::string, inputLineOpt>> {};

TEST_P(FakeCmdLineOptTest, GTestClass) // Test the FakeCmdLine class
{
  auto inputStruct = GetParam().second;
  FakeCmdLineOpt FakeOpt(inputStruct.arguments.begin(),inputStruct.arguments.end());
  size_t vectorLength = inputStruct.arguments.size();

  EXPECT_EQ(vectorLength+1,FakeOpt.argc());
  EXPECT_EQ(NULL,FakeOpt.argv()[vectorLength+1]);
  EXPECT_STREQ("myProgramName",FakeOpt.argv()[0])<<FakeOpt.argv()[0]; 
  for (size_t i=1; i<=vectorLength; i++)
    EXPECT_STREQ(inputStruct.arguments[i-1],FakeOpt.argv()[i]);
}

TEST_P(FakeCmdLineOptTest, ipcCopyFileClassCreator) // Test the process from commande line arguments to protocol and filepath
{
  auto inputStruct = GetParam().second;
  FakeCmdLineOpt FakeOpt(inputStruct.arguments.begin(),inputStruct.arguments.end());
  ipcParameters testOptions(FakeOpt.argc(), FakeOpt.argv());
  EXPECT_EQ(inputStruct.protocol, testOptions.getProtocol());
  EXPECT_STREQ(inputStruct.filepath, testOptions.getFilePath());
}

INSTANTIATE_TEST_SUITE_P(
  SelectionOfVariousInputLineCommandOptions,
  FakeCmdLineOptTest,
  ::testing::ValuesIn(inputMap),
  [](const ::testing::TestParamInfo<FakeCmdLineOptTest::ParamType> &info) {
    return info.param.first;
});