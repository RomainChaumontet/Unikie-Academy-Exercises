#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/lib/IpcCopyFile.h"
#include "../src/lib/IpcPipe.h"
#include "Gtest_Ipc.h"
#include <deque>
#include <vector>
#include <sstream>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using ::testing::Eq;
using ::testing::Ne;
using ::testing::Lt;
using ::testing::StrEq;
using ::testing::IsTrue;
using ::testing::IsFalse;
using ::testing::StartsWith;


std::vector<char> randomData = getRandomData(rand() % 4096);



class PipeTestSendFile : public PipeSendFile
{
    public:
        void modifyBuffer(std::vector<char> &input_pipe)
        {
            bufferSize_ = input_pipe.size();
            buffer_.resize(input_pipe.size());
            buffer_=input_pipe;
        };

        void modifyBuffer(const std::string &data)
        {
            bufferSize_ = data.size();
            buffer_ = std::vector<char> (data.begin(), data.end());
            return;
        };
};

class PipeTestReceiveFile : public PipeReceiveFile
{
    public:
        std::vector<char> getBuffer()
        {
            return buffer_;
        }
};


///////////////////// Test PipeSendFiles Constructor and Destructor //////////////////////
void ThreadPipeSendFileConstructor(void)
{
    ASSERT_NO_THROW(PipeSendFile myPipe);
}

void ThreadPipeSendFileonstructor2(void)
{
    while(!checkIfFileExists("ipcCopyFile"))
    {
        usleep(50);
    }
    std::fstream connect2Pipe;
    connect2Pipe.open("ipcCopyFile",std::ios::in | std::ios::binary);
    ASSERT_THAT(connect2Pipe.is_open(), IsTrue);
    connect2Pipe.close();
}

TEST(PipeSendFile,ConstructorAndDestructor)
{
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadPipeSendFileConstructor);
    start_pthread(&mThreadID2,ThreadPipeSendFileonstructor2);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ASSERT_THAT(checkIfFileExists("ipcCopyFile"),IsFalse());
}



///////////////////// Test PipeSendFiles sending text data //////////////////////

void ThreadPipeSendFilesyncIPCAndBufferTextData(void)
{
    PipeTestSendFile myPipe;
    std::string message = "This is a test.";
    myPipe.modifyBuffer(message);
    ASSERT_NO_THROW(myPipe.syncIPCAndBuffer());

}

void ThreadPipeSendFilesyncIPCAndBufferTextData2(void)
{
    while(!checkIfFileExists("ipcCopyFile"))
    {
        usleep(50);
    }
    std::fstream connect2Pipe;
    connect2Pipe.open("ipcCopyFile",std::ios::in | std::ios::binary);
    ASSERT_THAT(connect2Pipe.is_open(), IsTrue);
    std::vector<char> buffer(4096);
    connect2Pipe.read(buffer.data(), 4096);
    buffer.resize(connect2Pipe.gcount());

    EXPECT_THAT(std::string (buffer.begin(), buffer.end()), StrEq("This is a test."));

    connect2Pipe.close();
}

TEST(PipeSendFile, syncIPCAndBufferTextData)
{
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadPipeSendFilesyncIPCAndBufferTextData);
    start_pthread(&mThreadID2,ThreadPipeSendFilesyncIPCAndBufferTextData2);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ASSERT_THAT(checkIfFileExists("ipcCopyFile"),IsFalse());
}


///////////////////// Test PipeSendFiles sending binary data //////////////////////

void ThreadPipeSendFilesyncIPCAndBufferBinaryData(void)
{
    PipeTestSendFile myPipe;
    myPipe.modifyBuffer(randomData);
    ASSERT_NO_THROW(myPipe.syncIPCAndBuffer());

}

void ThreadPipeSendFilesyncIPCAndBufferBinaryData2(void)
{
    while(!checkIfFileExists("ipcCopyFile"))
    {
        usleep(50);
    }
    std::fstream connect2Pipe;
    connect2Pipe.open("ipcCopyFile",std::ios::in | std::ios::binary);
    ASSERT_THAT(connect2Pipe.is_open(), IsTrue);
    std::vector<char> buffer(4096);
    connect2Pipe.read(buffer.data(), 4096);
    buffer.resize(connect2Pipe.gcount());

    EXPECT_THAT(std::string (buffer.begin(), buffer.end()), StrEq(std::string (randomData.begin(), randomData.end())));

    connect2Pipe.close();
}

TEST(PipeSendFile, syncIPCAndBufferBinaryData)
{
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadPipeSendFilesyncIPCAndBufferBinaryData);
    start_pthread(&mThreadID2,ThreadPipeSendFilesyncIPCAndBufferBinaryData2);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ASSERT_THAT(checkIfFileExists("ipcCopyFile"),IsFalse());
}

///////////////////// Test PipeSendFiles sending multiple binary data //////////////////////

void ThreadPipeSendFilesyncIPCAndBufferMultipleBinaryData(void)
{
    PipeTestSendFile myPipe;
    myPipe.modifyBuffer(randomData);
    for (int i=0; i<10; i++)
        ASSERT_NO_THROW(myPipe.syncIPCAndBuffer());

}

void ThreadPipeSendFilesyncIPCAndBufferMultipleBinaryData2(void)
{
    while(!checkIfFileExists("ipcCopyFile"))
    {
        usleep(50);
    }
    std::fstream connect2Pipe;
    connect2Pipe.open("ipcCopyFile",std::ios::in | std::ios::binary);
    ASSERT_THAT(connect2Pipe.is_open(), IsTrue);
    std::vector<char> buffer;

    for (int i=0; i<10; i++)
    {
        std::vector<char> (randomData.size()).swap(buffer);
        connect2Pipe.read(buffer.data(), randomData.size());
        buffer.resize(connect2Pipe.gcount());
        EXPECT_THAT(std::string (buffer.begin(), buffer.end()), StrEq(std::string (randomData.begin(), randomData.end())));
    }
   
    connect2Pipe.close();
}

TEST(PipeSendFile, syncIPCAndBufferMultipleBinaryData)
{
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadPipeSendFilesyncIPCAndBufferMultipleBinaryData);
    start_pthread(&mThreadID2,ThreadPipeSendFilesyncIPCAndBufferMultipleBinaryData2);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ASSERT_THAT(checkIfFileExists("ipcCopyFile"),IsFalse());
}

///////////////////// Test PipeSendFiles read and send a file //////////////////////
void ThreadReadAndSend(void)
{
    PipeTestSendFile myPipe;
    myPipe.syncFileWithIPC("input_pipe.dat");
}

void ThreadReadAndSend2(void)
{

    while(!checkIfFileExists("ipcCopyFile"))
    {
        usleep(50);
    }

    FileManipulationClassWriter Receiver;
    ASSERT_NO_THROW(Receiver.openFile("output_pipe.dat"));
    std::fstream connect2Pipe;
    connect2Pipe.open("ipcCopyFile",std::ios::in | std::ios::binary);
    ASSERT_THAT(connect2Pipe.is_open(), IsTrue);
    std::vector<char> buffer;

    do
    {
        std::vector<char> (Receiver.getBufferSize()).swap(buffer);
        connect2Pipe.read(buffer.data(), buffer.size());
        buffer.resize(connect2Pipe.gcount());
        Receiver.modifyBufferToWrite(buffer);
        Receiver.syncFileWithBuffer();
    }
    while ((connect2Pipe.gcount() > 0) ||  checkIfFileExists("ipcCopyFile"));
     
   
    connect2Pipe.close();
}

TEST(PipeSendFile, ReadAndSend)
{
    CreateRandomFile Randomfile("input_pipe.dat",5,1);
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadReadAndSend);
    start_pthread(&mThreadID2,ThreadReadAndSend2);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ASSERT_THAT(checkIfFileExists("ipcCopyFile"),IsFalse());
    ASSERT_THAT(compareFiles("input_pipe.dat","output_pipe.dat"), IsTrue());
    remove("output_pipe.dat");
}


///////////////////// Test PipeReceiveFiles Constructor and Destructor //////////////////////
void ThreadPipeReceiveFileConstructor(void)
{
    CaptureStream stdcout{std::cout};
    ASSERT_NO_THROW(PipeReceiveFile myPipe);
    EXPECT_THAT(stdcout.str(), StartsWith("Waiting for ipc_sendfile.\n"));
}

void ThreadPipeReceiveFileonstructor2(void)
{
    std::fstream create2Pipe;
    unlink("ipcCopyFile");

    ASSERT_THAT(mkfifo("ipcCopyFile",S_IRWXU | S_IRWXG), Ne(-1));

    create2Pipe.open("ipcCopyFile",std::ios::out | std::ios::binary);
    ASSERT_THAT(create2Pipe.is_open(), IsTrue);
    create2Pipe.close();
    unlink("ipcCopyFile");
}

TEST(PipeReceiveFile,ConstructorAndDestructor)
{
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadPipeReceiveFileConstructor);
    usleep(50);
    start_pthread(&mThreadID2,ThreadPipeReceiveFileonstructor2);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ASSERT_THAT(checkIfFileExists("ipcCopyFile"),IsFalse());
}

///////////////////// Test PipeReceiveFiles Receiving Text Data//////////////////////
void ThreadPipeReceiveText(void)
{
    CaptureStream stdcout{std::cout};
    PipeTestReceiveFile myPipe;
    myPipe.syncIPCAndBuffer();
    std::vector<char> output;
    myPipe.getBuffer().swap(output);
    EXPECT_THAT(std::string (output.begin(), output.end()), StrEq("This is a test."));
}

void ThreadPipeReceiveText2(void)
{
    std::fstream create2Pipe;
    unlink("ipcCopyFile");

    ASSERT_THAT(mkfifo("ipcCopyFile",S_IRWXU | S_IRWXG), Ne(-1));

    create2Pipe.open("ipcCopyFile",std::ios::out | std::ios::binary);
    ASSERT_THAT(create2Pipe.is_open(), IsTrue);

    create2Pipe.write("This is a test.", strlen("This is a test."));
    create2Pipe.close();
    unlink("ipcCopyFile");
}

TEST(PipeReceiveFile,PipeReceiveText)
{
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID2,ThreadPipeReceiveText2);
    start_pthread(&mThreadID1,ThreadPipeReceiveText);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ASSERT_THAT(checkIfFileExists("ipcCopyFile"),IsFalse());
}


///////////////////// Test PipeReceiveFiles Receiving Binary Data//////////////////////
void ThreadPipeReceiveBinary(void)
{
    CaptureStream stdcout{std::cout};
    PipeTestReceiveFile myPipe;
    myPipe.syncIPCAndBuffer();
    std::vector<char> output;
    myPipe.getBuffer().swap(output);
    EXPECT_THAT(output.data(), StrEq(randomData.data()));
}

void ThreadPipeReceiveBinary2(void)
{
    std::fstream create2Pipe;
    unlink("ipcCopyFile");

    ASSERT_THAT(mkfifo("ipcCopyFile",S_IRWXU | S_IRWXG), Ne(-1));

    create2Pipe.open("ipcCopyFile",std::ios::out | std::ios::binary);
    ASSERT_THAT(create2Pipe.is_open(), IsTrue);

    create2Pipe.write(randomData.data(), randomData.size());
    create2Pipe.close();
    unlink("ipcCopyFile");
}

TEST(PipeReceiveFile,PipeReceiveBinary)
{
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID2,ThreadPipeReceiveBinary2);
    start_pthread(&mThreadID1,ThreadPipeReceiveBinary);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ASSERT_THAT(checkIfFileExists("ipcCopyFile"),IsFalse());
}


///////////////////// Test PipeReceiveFiles Receiving and Write//////////////////////
void ThreadPipeReceiveReceiveAndWrite(void)
{
    CaptureStream stdcout{std::cout};
    PipeTestReceiveFile myPipe;
    myPipe.syncFileWithIPC("output_pipe.dat");
}

void ThreadPipeReceiveReceiveAndWrite2(void)
{
    FileManipulationClassReader myReader;
    std::fstream create2Pipe;
    unlink("ipcCopyFile");

    ASSERT_THAT(mkfifo("ipcCopyFile",S_IRWXU | S_IRWXG), Ne(-1));

    create2Pipe.open("ipcCopyFile",std::ios::out | std::ios::binary);
    ASSERT_THAT(create2Pipe.is_open(), IsTrue);

    int filesize = returnFileSize("input_pipe.dat");
    int datasent = 0;
    myReader.openFile("input_pipe.dat");

    while (datasent < filesize)
    {
        myReader.syncFileWithBuffer();
        create2Pipe.write(myReader.getBufferRead().data(), myReader.getBufferSize());
        datasent += myReader.getBufferSize();
    }
    create2Pipe.close();
    unlink("ipcCopyFile");
}

TEST(PipeReceiveFile,PipeReceiveReceiveAndWrite)
{
    CreateRandomFile Randomfile("input_pipe.dat",1,1);
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID2,ThreadPipeReceiveReceiveAndWrite2);
    start_pthread(&mThreadID1,ThreadPipeReceiveReceiveAndWrite);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ASSERT_THAT(checkIfFileExists("ipcCopyFile"),IsFalse());
    EXPECT_THAT(compareFiles("input_pipe.dat","output_pipe.dat"), IsTrue);
    remove("output_pipe.dat");
}


///////////////////// Test PipeReceiveFile and PipeSendFile//////////////////////
void ThreadPipeReceiveFile(void)
{
    CaptureStream stdcout{std::cout};
    PipeReceiveFile myReceiver;
    myReceiver.syncFileWithIPC("output_pipe.dat");
}

void ThreadPipeSendFile(void)
{
    PipeSendFile mySender;
    mySender.syncFileWithIPC("input_pipe.dat");
}

TEST(PipeTest,FullImplementation)
{
    CreateRandomFile Randomfile("input_pipe.dat",1,1);
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID2,ThreadPipeSendFile);
    start_pthread(&mThreadID1,ThreadPipeReceiveFile);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ASSERT_THAT(checkIfFileExists("ipcCopyFile"),IsFalse());
    EXPECT_THAT(compareFiles("input_pipe.dat","output_pipe.dat"), IsTrue);
    remove("output_pipe.dat");
}




