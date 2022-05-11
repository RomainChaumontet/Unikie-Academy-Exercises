#include "Gtest_Ipc.h"


using ::testing::Eq;
using ::testing::Ne;
using ::testing::Lt;
using ::testing::StrEq;
using ::testing::IsTrue;
using ::testing::IsFalse;
using ::testing::StartsWith;
using ::testing::EndsWith;
using ::testing::ContainsRegex;
using ::testing::Ge;
using ::testing::Le;



std::map<std::string, inputLineOpt> inputMap =
{
  {"OK_Help",
        inputLineOpt {
          protocolList::HELP,
          NULL,
          std::vector<const char*> {"--help"}
          }},
  {"OK_Queue_File_NameOfFile",
        inputLineOpt {
          protocolList::QUEUE,
          "nameOfFile",
          std::vector<const char*> {"--queue", "--file","nameOfFile"},
          "/QueueIPC"
          }},
  {"OK_Pipe_File_NameOfFile",
      inputLineOpt {
        protocolList::PIPE,
        "nameOfFile",
        std::vector<const char*> {"--pipe", "--file","nameOfFile"},
        "PipeIPC"
        }},
  {"OK_Shm_File_NameOfFile",
        inputLineOpt {
          protocolList::SHM,
          "nameOfFile",
          std::vector<const char*> {"--shm", "--file","nameOfFile"},
          "/ShmIPC"
          }},
  {"OK_Queue_IPCName_File_NameOfFile",
        inputLineOpt {
          protocolList::QUEUE,
          "nameOfFile",
          std::vector<const char*> {"--queue", "/myQueue", "--file","nameOfFile"},
          "/myQueue"
          }},
  {"OK_Queue_IPCNameMissingSlash_File_NameOfFile",
        inputLineOpt {
          protocolList::QUEUE,
          "nameOfFile",
          std::vector<const char*> {"--queue", "myQueue", "--file","nameOfFile"},
          "/myQueue"
          }},
  {"OK_Queue_IPCNameTooLong_File_NameOfFile",
        inputLineOpt {
          protocolList::QUEUE,
          "nameOfFile",
          std::vector<const char*> {
              "--queue",
              "15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak",
              "--file",
              "nameOfFile"},
          "/QueueIPC"
          }},
  {"OK_Queue_IPCNameWith2Slash_File_NameOfFile",
        inputLineOpt {
          protocolList::QUEUE,
          "nameOfFile",
          std::vector<const char*> {
              "--queue",
              "/534/k",
              "--file",
              "nameOfFile"},
          "/QueueIPC"
          }},
  {"OK_Pipe_IPCName_File_NameOfFile",
      inputLineOpt {
        protocolList::PIPE,
        "nameOfFile",
        std::vector<const char*> {"--pipe", "myPipe", "--file","nameOfFile"},
        "myPipe"
        }},
  {"OK_Pipe_IPCNameTooLong_File_NameOfFile",
        inputLineOpt {
          protocolList::PIPE,
          "nameOfFile",
          std::vector<const char*> {
              "--pipe",
              "15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak",
              "--file",
              "nameOfFile"},
          "PipeIPC"
          }},
  {"OK_Shm_IPCName_File_NameOfFile",
        inputLineOpt {
          protocolList::SHM,
          "nameOfFile",
          std::vector<const char*> {"--shm", "/myShm", "--file","nameOfFile"},
          "/myShm"
          }},
  {"OK_Shm_IPCNameMissingSlash_File_NameOfFile",
        inputLineOpt {
          protocolList::SHM,
          "nameOfFile",
          std::vector<const char*> {"--shm", "myShm", "--file","nameOfFile"},
          "/myShm"
          }},
  {"OK_Shm_IPCNameTooLong_File_NameOfFile",
        inputLineOpt {
          protocolList::SHM,
          "nameOfFile",
          std::vector<const char*> {
              "--shm",
              "15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak15616165wafwadfawjbgghejeak",
              "--file",
              "nameOfFile"},
          "/ShmIPC"
          }},
  {"OK_File_NameOfFile_Queue",
        inputLineOpt {
          protocolList::QUEUE,
          "nameOfFile",
          std::vector<const char*> {"--file", "nameOfFile","--queue"},
          "/QueueIPC"
          }},
  {"OK_File_NameOfFile_Pipe",
        inputLineOpt {
          protocolList::PIPE,
          "nameOfFile",
          std::vector<const char*> {"--file", "nameOfFile","--pipe"},
          "PipeIPC"
          }},
  {"OK_File_NameOfFile_Shm",
        inputLineOpt {
          protocolList::SHM,
          "nameOfFile",
          std::vector<const char*> {"--file", "nameOfFile","--shm"},
          "/ShmIPC"
          }},
  {"NOK_",
        inputLineOpt {
          protocolList::NONE,
          NULL,
          std::vector<const char*> {""}
          }},
  {"NOK_hqasjbta",
        inputLineOpt {
          protocolList::ERROR,
          NULL,
          std::vector<const char*> {"-hqasjbta"}
          }},
  {"NOK_randomArgument",
        inputLineOpt {
          protocolList::ERROR,
          NULL,
          std::vector<const char*> {"--randomArgument"}
          }},
  {"NOK_queue",
        inputLineOpt {
          protocolList::ERROR,
          NULL,
          std::vector<const char*> {"--queue"}
          }},
  {"NOK_shm",
        inputLineOpt {
          protocolList::ERROR,
          NULL,
          std::vector<const char*> {"--shm"}
          }},
  {"NOK_pipe",
        inputLineOpt {
          protocolList::ERROR,
          NULL,
          std::vector<const char*> {"--pipe"}
          }},
  {"NOK_file",
        inputLineOpt {
          protocolList::ERROR,
          NULL,
          std::vector<const char*> {"--file"}
          }},
  {"NOK_file_nameOfFile",
        inputLineOpt {
          protocolList::NONE,
          "nameOfFile.extension",
          std::vector<const char*> {"--file", "nameOfFile.extension"}
          }},
  {"NOK_queue_file",
        inputLineOpt {
          protocolList::ERROR,
          NULL,
          std::vector<const char*> {"--queue","--file"}
          }},
  {"NOK_pipe_file",
        inputLineOpt {
          protocolList::ERROR,
          NULL,
          std::vector<const char*> {"--pipe","--file"}
          }},
  {"NOK_shm_file",
        inputLineOpt {
          protocolList::ERROR,
          NULL,
          std::vector<const char*> {"--shm","--file"}
          }},
  {"NOK_pipe_file_nameOfFile_shm",
        inputLineOpt {
          protocolList::ERROR,
          "nameOfFile",
          std::vector<const char*> {"--pipe","--file","nameOfFile","--shm"}
          }},
  {"NOK_queue_file_nameOfFile_pipe",
        inputLineOpt {
          protocolList::ERROR,
          "nameOfFile",
          std::vector<const char*> {"--queue","--file","nameOfFile","--pipe"}
          }}
};

class FakeCmdLineOptTest : public ::testing::TestWithParam<std::pair<const std::string, inputLineOpt>> {};


TEST_P(FakeCmdLineOptTest, IpcParameters) // Test the process from commande line arguments to protocol and filepath
{
    HandyFunctions myToolBox;
    auto inputStruct = GetParam().second;
    FakeCmdLineOpt FakeOpt(inputStruct.arguments.begin(),inputStruct.arguments.end());
    if (inputStruct.protocol == protocolList::ERROR || inputStruct.protocol == protocolList::NONE)
    {
        EXPECT_THROW(IpcParameters(FakeOpt.argc(), FakeOpt.argv(), &myToolBox), arguments_exception);
    }
    else
    {
        IpcParameters testOptions(FakeOpt.argc(), FakeOpt.argv(), &myToolBox);
        EXPECT_THAT(testOptions.getProtocol(), Eq(inputStruct.protocol));
        if (testOptions.getProtocol() != protocolList::HELP)
        {
            EXPECT_THAT(testOptions.getFilePath(), StrEq(inputStruct.filepath));
            EXPECT_THAT(testOptions.getIpcNames().at(testOptions.getProtocol()), StrEq(inputStruct.ipcPath)); 
        }
    }
}


INSTANTIATE_TEST_SUITE_P(
  TestVariousArguments,
  FakeCmdLineOptTest,
  ::testing::ValuesIn(inputMap),
  [](const ::testing::TestParamInfo<FakeCmdLineOptTest::ParamType> &info) {
    return info.param.first;
});