#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/lib/IpcCopyFile.h"
#include "../src/lib/IpcQueue.h"
#include "Gtest_Ipc.h"
#include <deque>
#include <vector>
#include <sstream>
#include <pthread.h>
#include <unistd.h>

using ::testing::Eq;
using ::testing::Ne;
using ::testing::StrEq;
using ::testing::IsTrue;
using ::testing::IsFalse;
using ::testing::StartsWith;

void ThreadExceedMaxMsgSend(void);  
void ThreadExceedMaxMsgReceive(void); 

class QueueTestSendFile : public QueueSendFile
{
    public:
        void modifyBuffer(std::vector<char> &input)
        {
            bufferSize_ = input.size();
            buffer_.resize(input.size());
            buffer_=input;
        };

        void modifyBuffer(const std::string &data)
        {
            bufferSize_ = data.size();
            buffer_ = std::vector<char> (data.begin(), data.end());
            return;
        };
        
};

class QueueTestReceiveFile : public QueueReceiveFile
{
    public:

        std::vector<char> getBuffer()
        {
            return buffer_;
        };
};


FileManipulationClassReader getSomeInfoQueue;

TEST(NoOtherProgram, SendQueueAlone)
{
    ASSERT_THROW(QueueSendFile myQueueObject(1),ipc_exception);
}
TEST(NoOtherProgram, ReceiveQueueAlone)
{
    QueueReceiveFile myQueueObject(1);
    ASSERT_THROW(myQueueObject.syncIPCAndBuffer(),ipc_exception);
}

TEST(BasicQueueCmd, OpenCloseQueue)
{
    std::string queueName = "/CopyDataThroughQueue";
    {
        QueueReceiveFile myRQueue;
        ASSERT_NO_THROW(QueueSendFile myQueueObject);
    } // Queue should not be opened anywhere (destructor)

    mqd_t queueTest = mq_open(queueName.c_str(),O_RDONLY);
    EXPECT_THAT(queueTest, Eq(-1)); //The queue does not exist -> error
    EXPECT_THAT(errno,Eq(ENOENT)); //The error is the queue does not exist.
}

TEST(BasicQueueCmd, QueueAlreadyOpened)
{
    
    //Without messages on it
    std::string queueName;
    {
        
        queueName = "/CopyDataThroughQueue";

        mqd_t queueTest = mq_open(
            queueName.c_str(),
                O_CREAT | O_WRONLY,
                S_IRWXG |S_IRWXU,NULL); //queue is open
        ASSERT_THAT(queueTest, Ne(-1));

        EXPECT_NO_THROW(QueueReceiveFile myQueueObject;);
        mq_close(queueTest);
        mq_unlink(queueName.c_str());
    }


    //With messages on it
    struct mq_attr queueAttrs;
    queueAttrs.mq_maxmsg = 10;
    queueAttrs.mq_msgsize = getSomeInfoQueue.getDefaultBufferSize();

    mqd_t queueTest = mq_open(
        queueName.c_str(),
            O_CREAT | O_WRONLY,
            S_IRWXG |S_IRWXU,NULL); //queue is open
    ASSERT_THAT(queueTest, Ne(-1));
    const char* dummYMsg = "Test";
    int status = mq_send(queueTest, dummYMsg, strlen(dummYMsg),5);
    ASSERT_THAT(status, Eq(0));
    status = mq_getattr(queueTest, &queueAttrs);
    ASSERT_THAT(status, Ne(-1));
    ASSERT_THAT(queueAttrs.mq_curmsgs,Eq(1));

    EXPECT_NO_THROW(QueueReceiveFile myQueueObject);
    QueueReceiveFile myQueueObject;
    status = mq_getattr(myQueueObject.getQueueDescriptor(), &queueAttrs);
    ASSERT_THAT(status, Ne(-1));
    ASSERT_THAT(queueAttrs.mq_curmsgs,Eq(0));

    //still in the first queue ?
    status = mq_getattr(queueTest, &queueAttrs);
    ASSERT_THAT(status, Ne(-1));
    ASSERT_THAT(queueAttrs.mq_curmsgs,Eq(1));

    mq_close(queueTest);
    mq_unlink(queueName.c_str());
}
    
TEST(SyncBuffAndQueue, SendQueue)
{
    QueueReceiveFile myRQueue; // just to create the queue
    std::string queueName = "/CopyDataThroughQueue";
    mqd_t queueTest;
    struct mq_attr queueAttrs;
    std::vector<char> buffer;
    size_t msgSize;
    std::string message = "This is a test.";
    unsigned int prio = 5;

    //Open the queue
    QueueTestSendFile MyQueueSend;
    queueTest = mq_open(queueName.c_str(), O_RDONLY);
    ASSERT_THAT(queueTest, Ne(-1));
    MyQueueSend.modifyBuffer(message);

    //sending a message in the Queue
    ASSERT_NO_THROW(MyQueueSend.syncIPCAndBuffer());
    EXPECT_THAT(mq_getattr(queueTest, &queueAttrs), Eq(0));
    EXPECT_THAT(queueAttrs.mq_curmsgs, Eq(1));

    //Check the message
    buffer.resize(queueAttrs.mq_msgsize);
    msgSize = mq_receive(queueTest, buffer.data(), queueAttrs.mq_msgsize, &prio);
    ASSERT_THAT(msgSize, Eq(message.size()));
    buffer.resize(msgSize);
    EXPECT_THAT(std::string (buffer.begin(), buffer.end()), StrEq(message));

    //Check if there is no other message
    EXPECT_THAT(mq_getattr(queueTest, &queueAttrs), Eq(0));
    EXPECT_THAT(queueAttrs.mq_curmsgs, Eq(0));

    
    /////////Try with binary data////////////////
    buffer.clear();
    std::vector<char> randomData = getRandomData();
    MyQueueSend.modifyBuffer(randomData);
    //sending a message in the Queue
    ASSERT_NO_THROW(MyQueueSend.syncIPCAndBuffer());
    EXPECT_THAT(mq_getattr(queueTest, &queueAttrs), Eq(0));
    EXPECT_THAT(queueAttrs.mq_curmsgs, Eq(1)); //one msg in the queue
    //Check the message
    std::vector<char>(queueAttrs.mq_msgsize).swap(buffer);
    size_t msgSize2 = mq_receive(queueTest, buffer.data(), queueAttrs.mq_msgsize, &prio);
    ASSERT_THAT(msgSize2, Eq(randomData.size()));
    buffer.resize(msgSize2);
    buffer.shrink_to_fit();
    EXPECT_TRUE(std::equal(randomData.begin(), randomData.end(), buffer.begin()));

    //Check if there is no other message
    EXPECT_THAT(mq_getattr(queueTest, &queueAttrs), Eq(0));
    EXPECT_THAT(queueAttrs.mq_curmsgs, Eq(0));

    //////////Try with random size binary data/////////
    buffer.clear();
    srand (time(NULL));
    std::vector<char> randomData2 = getRandomData();
    MyQueueSend.modifyBuffer(randomData2);
    //sending a message in the Queue
    ASSERT_NO_THROW(MyQueueSend.syncIPCAndBuffer());
    EXPECT_THAT(mq_getattr(queueTest, &queueAttrs), Eq(0));
    EXPECT_THAT(queueAttrs.mq_curmsgs, Eq(1)); //one msg in the queue
    //Check the message
    buffer.resize(queueAttrs.mq_msgsize);
    size_t msgSize3 = mq_receive(queueTest, buffer.data(), queueAttrs.mq_msgsize, &prio);
    ASSERT_THAT(msgSize3, Eq(randomData2.size())); 
    buffer.resize(msgSize3);
    for (size_t i=0; i< randomData2.size(); i++)
    {
        EXPECT_THAT(buffer[i], Eq(randomData2[i]));
    }

    mq_close(queueTest);
    mq_unlink(queueName.c_str());
}

TEST(SyncBuffAndQueue, ExceedMaxMsg)
{
    pthread_t mThreadID1, mThreadID2;

    QueueReceiveFile myRQueue; // just to create the queue
    start_pthread(&mThreadID1,ThreadExceedMaxMsgSend);
    usleep(50000);
    start_pthread(&mThreadID2,ThreadExceedMaxMsgReceive);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
}

void ThreadExceedMaxMsgSend(void)
{
    QueueTestSendFile MyQueueSend;
    std::string message = "This is a test.";
    struct mq_attr queueAttrs;

    MyQueueSend.modifyBuffer(message);

    for (int i=1; i<11 ; i++)
    {
        ASSERT_NO_THROW(MyQueueSend.syncIPCAndBuffer());
        EXPECT_THAT(mq_getattr(MyQueueSend.getQueueDescriptor(), &queueAttrs), Eq(0));
        EXPECT_THAT(queueAttrs.mq_curmsgs, Eq(i));
    }

    ASSERT_NO_THROW(MyQueueSend.syncIPCAndBuffer());
    EXPECT_THAT(mq_getattr(MyQueueSend.getQueueDescriptor(), &queueAttrs), Eq(0));
    EXPECT_THAT(queueAttrs.mq_curmsgs, Eq(10));
}

void ThreadExceedMaxMsgReceive(void)
{
    
    std::string queueName = "/CopyDataThroughQueue";
    mqd_t queueTest;
    struct mq_attr queueAttrs;
    std::vector<char> buffer;
    size_t msgSize;
    unsigned int prio = 5;

    queueTest = mq_open(queueName.c_str(), O_RDONLY);
    ASSERT_THAT(queueTest, Ne(-1));

    //Expect 10 msg in the queue
    EXPECT_THAT(mq_getattr(queueTest, &queueAttrs), Eq(0));
    EXPECT_THAT(queueAttrs.mq_curmsgs, Eq(10));

    //get one msg, expect 10 in the queue
    buffer.resize(queueAttrs.mq_msgsize);
    msgSize = mq_receive(queueTest, buffer.data(), queueAttrs.mq_msgsize, &prio);
    EXPECT_THAT(msgSize, Ne(-1));
    buffer.resize(msgSize);
    EXPECT_THAT(mq_getattr(queueTest, &queueAttrs), Eq(0));
    EXPECT_THAT(queueAttrs.mq_curmsgs, Eq(10));

    usleep(100);
    for (int i=10; i>0 ; i--)
    {
        buffer.resize(queueAttrs.mq_msgsize);
        msgSize = mq_receive(queueTest, buffer.data(), queueAttrs.mq_msgsize, &prio);
        EXPECT_THAT(msgSize, Ne(-1));
        buffer.resize(msgSize);
        EXPECT_THAT(mq_getattr(queueTest, &queueAttrs), Eq(0));
        EXPECT_THAT(queueAttrs.mq_curmsgs, Eq(i-1));
    }

    mq_close(queueTest);
    mq_unlink(queueName.c_str());
}

TEST(SyncBuffAndQueue, ReadSendQueueSimpleMessage)
{
    QueueReceiveFile myRQueue; // just to create the queue
    FileManipulationClassWriter writingToAFile;
    QueueSendFile myQueueSend;
    std::string data = "I expect these data will be send in the Queue.\n";
    std::string filename = "TmpFile.txt";
    writingToAFile.modifyBufferToWrite(data);
    ASSERT_NO_THROW(writingToAFile.openFile(filename)); 
    EXPECT_NO_THROW(writingToAFile.syncFileWithBuffer());
    writingToAFile.closeFile();

    EXPECT_NO_THROW(myQueueSend.openFile(filename));
    EXPECT_NO_THROW(myQueueSend.syncFileWithBuffer());
    EXPECT_NO_THROW(myQueueSend.syncIPCAndBuffer());


    std::string queueName = "/CopyDataThroughQueue";
    mqd_t queueTest;
    struct mq_attr queueAttrs;
    std::vector<char> buffer;
    size_t msgSize;
    unsigned int prio = 5;
    queueTest = mq_open(queueName.c_str(), O_RDONLY);
    ASSERT_THAT(queueTest, Ne(-1));

    //Check if there is one message
    EXPECT_THAT(mq_getattr(queueTest, &queueAttrs), Eq(0));
    EXPECT_THAT(queueAttrs.mq_curmsgs, Eq(1));

    buffer.resize(queueAttrs.mq_msgsize);
    msgSize = mq_receive(queueTest, buffer.data(), queueAttrs.mq_msgsize, &prio);
    ASSERT_THAT(msgSize, Eq(myQueueSend.getBufferSize()));
    buffer.resize(msgSize);
    EXPECT_THAT(std::string (buffer.begin(), buffer.end()), StrEq(data));

    mq_close(queueTest);
    mq_unlink(queueName.c_str());

    remove(filename.c_str());

}

TEST(SyncBuffAndQueue, ReadSendQueueComplexMessage)
{
    QueueReceiveFile myRQueue; // just to create the queue
    QueueSendFile myQueueSend;
    FileManipulationClassWriter writingToAFile;
    std::string fileinput = "input.dat";
    CreateRandomFile randomFile {fileinput,5, 1};
    ASSERT_THAT(checkIfFileExists(fileinput), IsTrue());
    std::string fileoutput = "output.dat";
    std::string queueName = "/CopyDataThroughQueue";
    mqd_t queueTest;
    struct mq_attr queueAttrs;
    std::vector<char> buffer;
    size_t msgSize;
    unsigned int prio = 5;
    ssize_t fileSize = returnFileSize(fileinput);
    ssize_t datasent = 0;

    ASSERT_NO_THROW(myQueueSend.openFile(fileinput));
    ASSERT_NO_THROW(writingToAFile.openFile(fileoutput));
    queueTest = mq_open(queueName.c_str(), O_RDONLY);
    ASSERT_THAT(queueTest, Ne(-1));

    while (datasent < fileSize)
    {
        ASSERT_NO_THROW(myQueueSend.syncFileWithBuffer());
        ASSERT_NO_THROW(myQueueSend.syncIPCAndBuffer());
        ASSERT_THAT(mq_getattr(queueTest,&queueAttrs),Eq(0));
        ASSERT_THAT(queueAttrs.mq_curmsgs,Eq(1)); // 1 msg on the queue

        buffer.clear();
        buffer.resize(queueAttrs.mq_msgsize);
        msgSize = mq_receive(queueTest, buffer.data(), queueAttrs.mq_msgsize, &prio);
        ASSERT_THAT(msgSize, Eq(myQueueSend.getBufferSize()));
        buffer.resize(msgSize);
        writingToAFile.modifyBufferToWrite(buffer);
        ASSERT_NO_THROW(writingToAFile.syncFileWithBuffer());
        datasent += msgSize;
    }

    writingToAFile.closeFile();
    myQueueSend.closeFile();

    EXPECT_THAT(compareFiles(fileoutput, fileinput), IsTrue());

    mq_close(queueTest);
    mq_unlink(queueName.c_str());

    remove(fileoutput.c_str());
}

TEST(BasicQueueCmd, SendQueueOpenCloseQueue)
{
    std::string queueName;
    mqd_t queueTest;
    {
        queueName = "/CopyDataThroughQueue";
        queueTest = mq_open(queueName.c_str(), O_CREAT | O_WRONLY,S_IRWXG |S_IRWXU,NULL);
        ASSERT_THAT(queueTest, Ne(-1));

        EXPECT_NO_THROW(QueueSendFile myQueueObject);

        mq_close(queueTest);
    }

    // the queue should be unlinked
    queueTest = mq_open(queueName.c_str(), O_RDONLY);
    EXPECT_THAT(queueTest, Eq(-1));
    EXPECT_THAT(errno, Eq(2)); //ENOENT

    {
        CaptureStream stdcout{std::cout};
        EXPECT_THROW(QueueSendFile myQueueObject{1},ipc_exception);
        EXPECT_THAT(stdcout.str(),StartsWith("Waiting to the ipc_receivefile.\n"));
    }

    mq_close(queueTest);
}

TEST(SyncBuffAndQueue, ReceiveQueue)
{
    mqd_t queueTest;
    std::string queueName = "/CopyDataThroughQueue";
    QueueTestReceiveFile myQueueObj;
    //open
    queueTest = mq_open(queueName.c_str(), O_WRONLY);
    ASSERT_THAT(queueTest, Ne(-1));

    // Text message
    std::string message = "This is a test.";
    mq_send(queueTest, message.c_str(), message.size(), 5);
    EXPECT_NO_THROW(myQueueObj.syncIPCAndBuffer());
    std::vector<char> output = myQueueObj.getBuffer();
    output.resize(myQueueObj.getBufferSize());
    EXPECT_THAT(std::string (output.begin(), output.end()), StrEq(message));

    //binary message
    std::vector<char> randomData = getRandomData();
    mq_send(queueTest, randomData.data(), randomData.size(), 5);
    EXPECT_NO_THROW(myQueueObj.syncIPCAndBuffer());
    output = myQueueObj.getBuffer();
    output.shrink_to_fit();
    EXPECT_TRUE(std::equal(randomData.begin(), randomData.end(), output.begin()));

    mq_close(queueTest);
}

TEST(SyncBuffandQueue, ReceiveQueueAndWrite)
{
    FileManipulationClassReader Reader;

    //files param
    std::string fileinput = "input.dat";
    CreateRandomFile randomFile {fileinput,5, 1};
    ASSERT_THAT(checkIfFileExists(fileinput), IsTrue());
    std::string fileoutput = "output.dat";

    //the sender param
    mqd_t queueTest;
    std::string queueName = "/CopyDataThroughQueue";
    std::vector<char> buffer;
    ssize_t fileSize = returnFileSize(fileinput);
    ssize_t datasent = 0;

    //Opening
    ASSERT_NO_THROW(Reader.openFile(fileinput));
    QueueReceiveFile myQueueObj;
    queueTest = mq_open(queueName.c_str(),O_WRONLY);
    ASSERT_THAT(queueTest, Ne(-1));
    ASSERT_NO_THROW(myQueueObj.openFile(fileoutput));

    //Loop
    while (datasent < fileSize)
    {
        std::vector<char> (getSomeInfoQueue.getDefaultBufferSize()).swap(buffer);
        ASSERT_NO_THROW(Reader.syncFileWithBuffer());
        buffer = Reader.getBufferRead();
        mq_send(queueTest, buffer.data(), buffer.size(), 5);
        datasent += buffer.size();
        ASSERT_NO_THROW(myQueueObj.syncIPCAndBuffer());
        ASSERT_NO_THROW(myQueueObj.syncFileWithBuffer());
    }
    
    Reader.closeFile();
    myQueueObj.closeFile();

    EXPECT_THAT(compareFiles(fileoutput, fileinput), IsTrue());

    remove(fileoutput.c_str());

    mq_close(queueTest);
}

void ThreadQueueSendManual(void)
{
    QueueSendFile myQueueSend;
    ASSERT_THAT(checkIfFileExists("input.dat"), IsTrue());
    ASSERT_NO_THROW(myQueueSend.openFile("input.dat"));
    ssize_t fileSize = returnFileSize("input.dat");
    ssize_t datasent = 0;

    while (datasent < fileSize)
    {
        ASSERT_NO_THROW(myQueueSend.syncFileWithBuffer());
        ASSERT_NO_THROW(myQueueSend.syncIPCAndBuffer());
        datasent += myQueueSend.getBufferSize();
    }
    // send an empty message
    ASSERT_NO_THROW(myQueueSend.syncFileWithBuffer());
    ASSERT_NO_THROW(myQueueSend.syncIPCAndBuffer());
}

void ThreadQueueReceiveManual(void)
{
    QueueReceiveFile myQueueReceive;
    ASSERT_NO_THROW(myQueueReceive.openFile("output.dat"));

    while (myQueueReceive.getBufferSize() > 0)
    {
        ASSERT_NO_THROW(myQueueReceive.syncIPCAndBuffer());
        ASSERT_NO_THROW(myQueueReceive.syncFileWithBuffer());
    }
}

TEST(QueueSendAndReceive, ManualLoop)
{
    std::string fileinput = "input.dat";
    std::string fileoutput = "output.dat";
    CreateRandomFile randomFile {fileinput,5, 1};

    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadQueueSendManual);
    start_pthread(&mThreadID2,ThreadQueueReceiveManual);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 


    EXPECT_THAT(compareFiles(fileoutput, fileinput), IsTrue());

    remove(fileinput.c_str());
    remove(fileoutput.c_str());
}

void ThreadQueueSendAuto(void)
{
    QueueSendFile myQueueSend;
    ASSERT_NO_THROW(myQueueSend.syncFileWithIPC("input.dat"));
}

void ThreadQueueReceiveAuto(void)
{
    QueueReceiveFile myQueueReceive;
    ASSERT_NO_THROW(myQueueReceive.syncFileWithIPC("output.dat"));
}
TEST(QueueSendAndReceive, UsingsyncFileWithIPC)
{
    mq_unlink("/CopyDataThroughQueue");
    std::string fileinput = "input.dat";
    std::string fileoutput = "output.dat";
    
    CreateRandomFile randomFile {fileinput,5, 1};

    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadQueueSendAuto);
    start_pthread(&mThreadID2,ThreadQueueReceiveAuto);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 


    EXPECT_THAT(compareFiles(fileoutput, fileinput), IsTrue());


    remove(fileoutput.c_str());
}

////////////////////// Killing a program: SendFile killed//////////////////////////


void ThreadQueueSendFileKilledSend(void)
{
    QueueSendFile myQueueSend;
    myQueueSend.openFile("input.dat");
    myQueueSend.sendHeader("input.dat");
    srand (time(NULL));
    int numberOfMessage = rand() % 20; //will end after a random number of message
    for (int i = 0; i<numberOfMessage; i++)
    {
        myQueueSend.syncFileWithBuffer();
        myQueueSend.syncIPCAndBuffer();
    }
}

void ThreadQueueSendFileKilledReceive(void)
{
    QueueReceiveFile myQueueReceive(2);
    ASSERT_THROW(myQueueReceive.syncFileWithIPC("output.dat"), ipc_exception);
}

TEST(KillingAProgram, QueueSendFileKilled)
{
    mq_unlink("/CopyDataThroughQueue");
    std::string fileinput = "input.dat";
    std::string fileoutput = "output.dat";
    
    CreateRandomFile randomFile {fileinput,5, 5};

    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadQueueSendFileKilledSend);
    start_pthread(&mThreadID2,ThreadQueueSendFileKilledReceive);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 


    remove(fileoutput.c_str());
}


////////////////////// Killing a program: Receivefile killed//////////////////////////

void ThreadQueueReceiveFileKilledSend(void)
{
    QueueSendFile myQueueSend(3);
    ASSERT_THROW(myQueueSend.syncFileWithIPC("input.dat"), ipc_exception);
}

void ThreadQueueReceiveFileKilledReceive(void)
{
    QueueReceiveFile myQueueReceive;
    srand (time(NULL));
    int numberOfMessage = rand() % 20 +10; //will end after a random number of message
    for (int i = 0; i<numberOfMessage; i++)
    {
        myQueueReceive.syncIPCAndBuffer();
    }
}

TEST(KillingAProgram, QueueReceiveFileKilled)
{
    mq_unlink("/CopyDataThroughQueue");
    std::string fileinput = "input.dat";
    
    CreateRandomFile randomFile {fileinput,5, 1};

    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID2,ThreadQueueReceiveFileKilledReceive);
    usleep(50);
    start_pthread(&mThreadID1,ThreadQueueReceiveFileKilledSend);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 

}


//////////////////////////// IPC Channel already exist ///////////////////////



void QueueReceiveFileSend(void)
{
    ASSERT_THROW(QueueSendFile myQueueObject, ipc_exception);
}

void QueueReceiveFileReceive(void)
{
    QueueReceiveFile myQueueReceive{1};
    ASSERT_THROW(myQueueReceive.syncFileWithIPC("output.dat"), ipc_exception);
}


TEST(ipcAlreadyExists, QueueExistWithMsg)
{
    std::string queueName = "/CopyDataThroughQueue";
    mqd_t queueTest = mq_open(
        queueName.c_str(),
            O_CREAT | O_WRONLY,
            S_IRWXG |S_IRWXU,NULL); //queue is open
    std::string data = "data";
    mq_send(queueTest, &data[0], data.size(), 5); //sending a message in the queue

    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,QueueReceiveFileSend);
    usleep(500);
    start_pthread(&mThreadID2,QueueReceiveFileReceive);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 

    mq_close(queueTest);
    mq_unlink("/CopyDataThroughQueue");
    remove("output.dat");
}