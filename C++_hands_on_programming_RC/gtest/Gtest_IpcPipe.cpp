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
#include <stdlib.h>
#include <time.h> 

using ::testing::Eq;
using ::testing::Ne;
using ::testing::Lt;
using ::testing::StrEq;
using ::testing::IsTrue;
using ::testing::IsFalse;
using ::testing::StartsWith;
using ::testing::EndsWith;

std::vector<char> randomData = getRandomData();

FileManipulationClassReader getSomeInfo;

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
        PipeTestReceiveFile(int maxAttempt):PipeReceiveFile(maxAttempt){};
        PipeTestReceiveFile():PipeTestReceiveFile(30){};
        std::vector<char> getBuffer()
        {
            buffer_.resize(bufferSize_);
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
    while(!checkIfFileExists("CopyDataThroughPipe"))
    {
        usleep(50);

    }
    std::fstream connect2Pipe;
    connect2Pipe.open("CopyDataThroughPipe",std::ios::in | std::ios::binary);
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
    ASSERT_THAT(checkIfFileExists("CopyDataThroughPipe"),IsFalse());
}


///////////////////////// Test PipeSendFiles only program launched ///////////////
TEST(NoOtherProgram,PipeSendFile)
{
    EXPECT_THROW(PipeSendFile myPipe(1), ipc_exception);
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
    while(!checkIfFileExists("CopyDataThroughPipe"))
    {
        usleep(50);
    }
    std::fstream connect2Pipe;
    connect2Pipe.open("CopyDataThroughPipe",std::ios::in | std::ios::binary);
    EXPECT_THAT(connect2Pipe.is_open(), IsTrue);
    std::vector<char> buffer(getSomeInfo.getDefaultBufferSize());
    connect2Pipe.read(buffer.data(), getSomeInfo.getDefaultBufferSize());
    buffer.resize(connect2Pipe.gcount());

    EXPECT_THAT(std::string (buffer.begin(), buffer.end()), StrEq("This is a test."));

    connect2Pipe.close();
    unlink("CopyDataThroughPipe");
}

TEST(PipeSendFile, syncIPCAndBufferTextData)
{
    
    remove("CopyDataThroughPipe");
    usleep(500);
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadPipeSendFilesyncIPCAndBufferTextData);
    start_pthread(&mThreadID2,ThreadPipeSendFilesyncIPCAndBufferTextData2);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ASSERT_THAT(checkIfFileExists("CopyDataThroughPipe"),IsFalse());
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
    while(!checkIfFileExists("CopyDataThroughPipe"))
    {
        usleep(50);
    }
    std::fstream connect2Pipe;
    connect2Pipe.open("CopyDataThroughPipe",std::ios::in | std::ios::binary);
    ASSERT_THAT(connect2Pipe.is_open(), IsTrue);
    std::vector<char> buffer(getSomeInfo.getDefaultBufferSize());
    connect2Pipe.read(buffer.data(), getSomeInfo.getDefaultBufferSize());
    buffer.resize(connect2Pipe.gcount());

    EXPECT_THAT(std::string (buffer.begin(), buffer.end()), StrEq(std::string (randomData.begin(), randomData.end())));

    connect2Pipe.close();
    unlink("CopyDataThroughPipe");
}

TEST(PipeSendFile, syncIPCAndBufferBinaryData)
{
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadPipeSendFilesyncIPCAndBufferBinaryData);
    start_pthread(&mThreadID2,ThreadPipeSendFilesyncIPCAndBufferBinaryData2);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ASSERT_THAT(checkIfFileExists("CopyDataThroughPipe"),IsFalse());
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
    while(!checkIfFileExists("CopyDataThroughPipe"))
    {
        usleep(50);
    }
    std::fstream connect2Pipe;
    connect2Pipe.open("CopyDataThroughPipe",std::ios::in | std::ios::binary);
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
    unlink("CopyDataThroughPipe");
}

TEST(PipeSendFile, syncIPCAndBufferMultipleBinaryData)
{
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadPipeSendFilesyncIPCAndBufferMultipleBinaryData);
    start_pthread(&mThreadID2,ThreadPipeSendFilesyncIPCAndBufferMultipleBinaryData2);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ASSERT_THAT(checkIfFileExists("CopyDataThroughPipe"),IsFalse());
}

///////////////////// Test PipeSendFiles read and send a file //////////////////////
void ThreadReadAndSend(void)
{
    PipeTestSendFile myPipe;
    ASSERT_NO_THROW(myPipe.syncFileWithIPC("input_pipe.dat"));
}

void ThreadReadAndSend2(void)
{
    while(!checkIfFileExists("CopyDataThroughPipe"))
    {
        usleep(50);
    }
    FileManipulationClassWriter Receiver;
    
    Receiver.openFile("output_pipe.dat");
    std::fstream connect2Pipe;
    connect2Pipe.open("CopyDataThroughPipe",std::ios::in | std::ios::binary);
    ASSERT_THAT(connect2Pipe.is_open(), IsTrue);

    std::vector<char> buffer;
    std::vector<size_t> header;
    buffer.resize(Receiver.getDefaultBufferSize());
    header.resize(Receiver.getDefaultBufferSize());

    connect2Pipe.read(reinterpret_cast<char*>(header.data()), Receiver.getDefaultBufferSize());

    size_t fileSize = header[1];
    size_t dataReceived =0;
    do
    {
        std::vector<char> (Receiver.getBufferSize()).swap(buffer);
        connect2Pipe.read(buffer.data(), buffer.size());
        buffer.resize(connect2Pipe.gcount());
        dataReceived += connect2Pipe.gcount();
        Receiver.modifyBufferToWrite(buffer);
        Receiver.syncFileWithBuffer();

    }
    while (dataReceived<fileSize);
     
   
    connect2Pipe.close();

    unlink("CopyDataThroughPipe");
}

TEST(PipeSendFile, ReadAndSend)
{
    remove("CopyDataThroughPipe");
    CreateRandomFile Randomfile("input_pipe.dat",5,1);
    pthread_t mThreadID3, mThreadID4;
    start_pthread(&mThreadID3,ThreadReadAndSend);
    usleep(50);
    start_pthread(&mThreadID4,ThreadReadAndSend2);
    ::pthread_join(mThreadID3, nullptr);
    ::pthread_join(mThreadID4, nullptr); 
    ASSERT_THAT(checkIfFileExists("CopyDataThroughPipe"),IsFalse());
    ASSERT_THAT(compareFiles("input_pipe.dat","output_pipe.dat"), IsTrue());
    remove("output_pipe.dat");
}


///////////////////// Test PipeReceiveFiles Constructor and Destructor //////////////////////
void ThreadPipeReceiveFileConstructor(void)
{
    ASSERT_NO_THROW(PipeReceiveFile myPipe);
}

void ThreadPipeReceiveFileonstructor2(void)
{
    std::fstream create2Pipe;
    unlink("CopyDataThroughPipe");

    ASSERT_THAT(mkfifo("CopyDataThroughPipe",S_IRWXU | S_IRWXG), Ne(-1));

    create2Pipe.open("CopyDataThroughPipe",std::ios::out | std::ios::binary);
    ASSERT_THAT(create2Pipe.is_open(), IsTrue);
    create2Pipe.close();
    unlink("CopyDataThroughPipe");
}

TEST(PipeReceiveFile,ConstructorAndDestructor)
{
    CaptureStream stdcout{std::cout};
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadPipeReceiveFileConstructor);
    usleep(50);
    start_pthread(&mThreadID2,ThreadPipeReceiveFileonstructor2);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    EXPECT_THAT(stdcout.str(), StartsWith("Waiting for ipc_sendfile.\n"));
    ASSERT_THAT(checkIfFileExists("CopyDataThroughPipe"),IsFalse());
}
///////////////////////// Test PipeReceiveFiles only program launched ///////////////
TEST(NoOtherProgram,PipeReceiveFile)
{
    EXPECT_THROW(PipeReceiveFile myPipe(1), ipc_exception);
}

///////////////////// Test PipeReceiveFiles Receiving Text Data//////////////////////
void ThreadPipeReceiveText(void)
{
    PipeTestReceiveFile myPipe;
    myPipe.getBuffer();//resizing buffer
    myPipe.syncIPCAndBuffer();
    std::vector<char> output;
    myPipe.getBuffer().swap(output);
    EXPECT_THAT(std::string (output.begin(), output.end()), StrEq("This is a test."));
}

void ThreadPipeReceiveText2(void)
{
    std::fstream create2Pipe;
    mkfifo("CopyDataThroughPipe",S_IRWXU | S_IRWXG);

    create2Pipe.open("CopyDataThroughPipe",std::ios::out | std::ios::binary);
    ASSERT_THAT(create2Pipe.is_open(), IsTrue);
    create2Pipe.write("This is a test.", strlen("This is a test."));
    create2Pipe.close();
    unlink("CopyDataThroughPipe");
}

TEST(PipeReceiveFile,PipeReceiveText)
{
    CaptureStream stdcout{std::cout};
    unlink("CopyDataThroughPipe");
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadPipeReceiveText);
    start_pthread(&mThreadID2,ThreadPipeReceiveText2);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ASSERT_THAT(checkIfFileExists("CopyDataThroughPipe"),IsFalse());
}


///////////////////// Test PipeReceiveFiles Receiving Binary Data//////////////////////
void ThreadPipeReceiveBinary(void)
{
    PipeTestReceiveFile myPipe;
    myPipe.getBuffer();
    myPipe.syncIPCAndBuffer();
    std::vector<char> output = myPipe.getBuffer();
    EXPECT_THAT(myPipe.getBuffer().data(), StrEq(randomData.data()));
}

void ThreadPipeReceiveBinary2(void)
{
    std::fstream create2Pipe;
    mkfifo("CopyDataThroughPipe",S_IRWXU | S_IRWXG);

    create2Pipe.open("CopyDataThroughPipe",std::ios::out | std::ios::binary);
    ASSERT_THAT(create2Pipe.is_open(), IsTrue);
    create2Pipe.write(randomData.data(), randomData.size());
    create2Pipe.close();
    unlink("CopyDataThroughPipe");
}

TEST(PipeReceiveFile,PipeReceiveBinary)
{
    CaptureStream stdcout{std::cout};
    unlink("CopyDataThroughPipe");
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID2,ThreadPipeReceiveBinary2);
    start_pthread(&mThreadID1,ThreadPipeReceiveBinary);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ASSERT_THAT(checkIfFileExists("CopyDataThroughPipe"),IsFalse());
}


///////////////////// Test PipeReceiveFiles Receiving and Write//////////////////////
void ThreadPipeReceiveReceiveAndWrite(void)
{
    ASSERT_NO_THROW(
        {
            PipeTestReceiveFile myPipe{2};
            myPipe.syncFileWithIPC("output_pipe.dat");
        });
}

void ThreadPipeReceiveReceiveAndWrite2(void)
{
    ASSERT_THAT(open("CopyDataThroughPipe",O_RDONLY), Eq(-1));
    FileManipulationClassReader myReader;
    std::fstream create2Pipe;

    mkfifo("CopyDataThroughPipe",S_IRWXU | S_IRWXG);

    create2Pipe.open("CopyDataThroughPipe",std::ios::out | std::ios::binary);
    ASSERT_THAT(create2Pipe.is_open(), IsTrue);

    int filesize = returnFileSize("input_pipe.dat");
    int datasent = 0;
    myReader.openFile("input_pipe.dat");
    Header header("input_pipe.dat",myReader.getDefaultBufferSize());
    create2Pipe.write(reinterpret_cast<char*>(header.getHeader().data()), myReader.getDefaultBufferSize());

    while (datasent < filesize)
    {
        myReader.syncFileWithBuffer();
        create2Pipe.write(myReader.getBufferRead().data(), myReader.getBufferSize());
        datasent += myReader.getBufferSize();
    }
    std::cout << " all data sent" << std::endl;
    create2Pipe.close();
    
    unlink("CopyDataThroughPipe");
}

TEST(PipeReceiveFile,PipeReceiveReceiveAndWrite)
{
    CaptureStream stdcout{std::cout}; //mute cout
    remove("CopyDataThroughPipe");
    usleep(50);
    CreateRandomFile Randomfile("input_pipe.dat",1,1);
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID2,ThreadPipeReceiveReceiveAndWrite2);
    start_pthread(&mThreadID1,ThreadPipeReceiveReceiveAndWrite);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ASSERT_THAT(checkIfFileExists("CopyDataThroughPipe"),IsFalse());
    EXPECT_THAT(compareFiles("input_pipe.dat","output_pipe.dat"), IsTrue);
    remove("output_pipe.dat");
}


///////////////////// Test PipeReceiveFile and PipeSendFile//////////////////////
void ThreadPipeReceiveFile(void)
{
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
    CaptureStream stdcout{std::cout}; //mute cout
    remove("CopyDataThroughPipe");
    CreateRandomFile Randomfile("input_pipe.dat",1,1);
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID2,ThreadPipeSendFile);
    start_pthread(&mThreadID1,ThreadPipeReceiveFile);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ASSERT_THAT(checkIfFileExists("CopyDataThroughPipe"),IsFalse());
    EXPECT_THAT(compareFiles("input_pipe.dat","output_pipe.dat"), IsTrue);
    remove("output_pipe.dat");
}



/////////////////////// Killing a program: receiveFile//////////////////

void ThreadPipeReceiveFileKilledReceive(void)
{
    PipeTestReceiveFile myReceiver(5);
    myReceiver.getBuffer();
    myReceiver.receiveHeader();
    myReceiver.syncIPCAndBuffer(); //just sync one time and after leave.
}

void ThreadPipeReceiveFileKilledSend(void)
{
    PipeSendFile mySender(3);
    ASSERT_THROW(mySender.syncFileWithIPC("input_pipe.dat"), ipc_exception);
}

TEST(KillingAProgram,PipeReceiveFileKilled)
{
    CaptureStream stderr(std::cerr);
    remove("CopyDataThroughPipe");
    CreateRandomFile Randomfile("input_pipe.dat",1,1);
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadPipeReceiveFileKilledReceive);
    start_pthread(&mThreadID2,ThreadPipeReceiveFileKilledSend);
    ::pthread_join(mThreadID2, nullptr); 
    ::pthread_join(mThreadID1, nullptr);
    remove("output_pipe.dat");
    remove("CopyDataThroughPipe");
    EXPECT_THAT(stderr.str(),StrEq("Error. Can't find the other program. Did it crash ?\n"));
}

/////////////////////// Killing a program: sendFile//////////////////

void ThreadPipeSendFileKilledReceive(void)
{
    PipeReceiveFile myReceiver(3);
    ASSERT_THROW(myReceiver.syncFileWithIPC("input_pipe.dat"), ipc_exception);
}

void ThreadPipeSendFileKilledSend(void)
{
    srand (time(NULL));
    PipeSendFile mySender(3);
    mySender.openFile("input_pipe.dat");
    int numberOfMessage = rand() % 20; //will end after a random number of message
    for (int i = 0; i<numberOfMessage; i++)
    {
        mySender.syncFileWithBuffer();
        mySender.syncIPCAndBuffer();
    }
}

TEST(KillingAProgram,PipeSendFileKilled)
{
    CaptureStream stdout(std::cout);
    remove("CopyDataThroughPipe");
    CreateRandomFile Randomfile("input_pipe.dat",10,10);
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadPipeSendFileKilledReceive);
    start_pthread(&mThreadID2,ThreadPipeSendFileKilledSend);
    ::pthread_join(mThreadID2, nullptr); 
    ::pthread_join(mThreadID1, nullptr);
    remove("output_pipe.dat");
    remove("CopyDataThroughPipe");
}


////////////////////////The IPC is used by another program/////////////////
void ThreadIPCUsedByAnotherProgramReceive(void)
{
    
    PipeReceiveFile myReceiver;
    ASSERT_THROW(myReceiver.syncFileWithIPC("output_pipe.dat"), ipc_exception);
}

void ThreadIPCUsedByAnotherProgramSend(void)
{
    ASSERT_THROW({
        PipeSendFile mySender;
        mySender.syncFileWithIPC("input_pipe.dat");
    },ipc_exception);
}

TEST(IPCUsedByAnotherProgram,PipeDoubleSender)
{
    remove("CopyDataThroughPipe");
    usleep(50);
    ASSERT_THAT(open("CopyDataThroughPipe",O_RDONLY), Eq(-1));
    CaptureStream stdcout{std::cout};
    CreateRandomFile Randomfile("input_pipe.dat",10,10);
    pthread_t mThreadID1, mThreadID2, mThreadID3;
    start_pthread(&mThreadID2,ThreadIPCUsedByAnotherProgramReceive);
    usleep(50);
    start_pthread(&mThreadID1,ThreadIPCUsedByAnotherProgramSend);
    start_pthread(&mThreadID3,ThreadIPCUsedByAnotherProgramSend);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ::pthread_join(mThreadID3, nullptr); 
    remove("output_pipe.dat");
}

void ThreadIPCUsedByAnotherProgram2Receive(void)
{
    PipeReceiveFile myReceiver;
    ASSERT_THROW(myReceiver.syncFileWithIPC("output_pipe.dat"), ipc_exception);
}

void ThreadIPCUsedByAnotherProgram1Send(void)
{
    PipeSendFile mySender;
    mySender.syncFileWithIPC("input_pipe.dat");
}

TEST(IPCUsedByAnotherProgram,PipeDoubleReceiver)
{
    CaptureStream stdcout{std::cout};
    remove("CopyDataThroughPipe");
    usleep(50);
    CreateRandomFile Randomfile("input_pipe.dat",10,10);
    pthread_t mThreadID1, mThreadID2, mThreadID3;
    start_pthread(&mThreadID1,ThreadIPCUsedByAnotherProgram2Receive);
    start_pthread(&mThreadID3,ThreadIPCUsedByAnotherProgram2Receive);
    start_pthread(&mThreadID2,ThreadIPCUsedByAnotherProgram1Send);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ::pthread_join(mThreadID3, nullptr); 
    remove("output_pipe.dat");
}
