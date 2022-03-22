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
        void changeMaxAttempt(int input)
        {
            maxAttempt_ = input;
        }

        std::vector<char> getBuffer()
        {
            return buffer_;
        };
};

TEST(BasicQueueCmd, SendQueueOpenCloseQueue)
{
    std::string queueName;
    {
        QueueSendFile myQueueObject;
        queueName = "/" + myQueueObject.getName();

        ASSERT_NO_THROW(myQueueObject.openIPC());
        mqd_t queueTest = mq_open(queueName.c_str(),O_RDONLY);
        EXPECT_THAT(queueTest, Ne(-1)); //The queue exists
        mq_close(queueTest);
    } // Queue should not be opened anywhere (destructor)

    mq_unlink(queueName.c_str()); //should destroy the queue

    mqd_t queueTest = mq_open(queueName.c_str(),O_RDONLY);
    EXPECT_THAT(queueTest, Eq(-1)); //The queue does not exist -> error
    EXPECT_THAT(errno,Eq(2)); //The error is the queue does not exist.
}

TEST(BasicQueueCmd, SendQueueQueueAlreadyOpened)
{
    //Without messages on it
    std::string queueName;
    QueueSendFile myQueueObject;
    queueName = "/" + myQueueObject.getName();

    mqd_t queueTest = mq_open(
        queueName.c_str(),
            O_CREAT | O_WRONLY,
            S_IRWXG |S_IRWXU,NULL); //queue is open
    ASSERT_THAT(queueTest, Ne(-1));

    EXPECT_NO_THROW(myQueueObject.openIPC());
    mq_close(queueTest);
    mq_unlink(queueName.c_str());


    //With messages on it
    struct mq_attr queueAttrs;
    queueAttrs.mq_maxmsg = 10;
    queueAttrs.mq_msgsize = 4096;

    queueTest = mq_open(
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

    EXPECT_NO_THROW(myQueueObject.openIPC());
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
    QueueTestSendFile MyQueueSend;
    std::string queueName = "/" + MyQueueSend.getName();
    mqd_t queueTest;
    struct mq_attr queueAttrs;
    std::vector<char> buffer;
    size_t msgSize;
    std::string message = "This is a test.";
    unsigned int prio = 5;

    //Open the queue
    ASSERT_NO_THROW(MyQueueSend.openIPC());
    queueTest = mq_open(queueName.c_str(), O_RDONLY);
    ASSERT_THAT(queueTest, Ne(-1));
    MyQueueSend.modifyBuffer(message);

    //sending a message in the Queue
    ASSERT_NO_THROW(MyQueueSend.syncIPCAndBuffer());
    EXPECT_THAT(mq_getattr(queueTest, &queueAttrs), Eq(0));
    EXPECT_THAT(queueAttrs.mq_curmsgs, Eq(1));

    //Check the message
    buffer.resize(queueAttrs.mq_msgsize);
    msgSize = mq_receive(queueTest, &buffer[0], queueAttrs.mq_msgsize, &prio);
    ASSERT_THAT(msgSize, Eq(message.size()));
    buffer.resize(msgSize);
    EXPECT_THAT(std::string (buffer.begin(), buffer.end()), StrEq(message));

    //Check if there is no other message
    EXPECT_THAT(mq_getattr(queueTest, &queueAttrs), Eq(0));
    EXPECT_THAT(queueAttrs.mq_curmsgs, Eq(0));

    
    /////////Try with binary data////////////////
    buffer.clear();
    std::vector<char> randomData = getRandomData(4096);
    MyQueueSend.modifyBuffer(randomData);
    //sending a message in the Queue
    ASSERT_NO_THROW(MyQueueSend.syncIPCAndBuffer());
    EXPECT_THAT(mq_getattr(queueTest, &queueAttrs), Eq(0));
    EXPECT_THAT(queueAttrs.mq_curmsgs, Eq(1)); //one msg in the queue
    //Check the message
    buffer.resize(queueAttrs.mq_msgsize);
    size_t msgSize2 = mq_receive(queueTest, &buffer[0], queueAttrs.mq_msgsize, &prio);
    ASSERT_THAT(msgSize2, Eq(4096));
    buffer.resize(msgSize2);
    EXPECT_THAT(&buffer[0], StrEq(&randomData[0]));

    //Check if there is no other message
    EXPECT_THAT(mq_getattr(queueTest, &queueAttrs), Eq(0));
    EXPECT_THAT(queueAttrs.mq_curmsgs, Eq(0));

    //////////Try with random size binary data/////////
    buffer.clear();
    ssize_t randomSize = rand() % 4096;
    std::vector<char> randomData2 = getRandomData(randomSize);
    MyQueueSend.modifyBuffer(randomData2);
    //sending a message in the Queue
    ASSERT_NO_THROW(MyQueueSend.syncIPCAndBuffer());
    EXPECT_THAT(mq_getattr(queueTest, &queueAttrs), Eq(0));
    EXPECT_THAT(queueAttrs.mq_curmsgs, Eq(1)); //one msg in the queue
    //Check the message
    buffer.resize(queueAttrs.mq_msgsize);
    msgSize2 = mq_receive(queueTest, &buffer[0], queueAttrs.mq_msgsize, &prio);
    ASSERT_THAT(msgSize2, Eq(randomSize));
    buffer.resize(msgSize2);
    EXPECT_THAT(&buffer[0], StrEq(&randomData2[0]));

    mq_close(queueTest);
    mq_unlink(queueName.c_str());
}

TEST(SyncBuffAndQueue, ExceedMaxMsg)
{
    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadExceedMaxMsgSend);
    usleep(50);
    start_pthread(&mThreadID2,ThreadExceedMaxMsgReceive);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
}

void ThreadExceedMaxMsgSend(void)
{
    QueueTestSendFile MyQueueSend;
    ASSERT_NO_THROW(MyQueueSend.openIPC());
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
    QueueTestSendFile MyQueue;
    std::string queueName = "/" + MyQueue.getName();
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
    msgSize = mq_receive(queueTest, &buffer[0], queueAttrs.mq_msgsize, &prio);
    EXPECT_THAT(msgSize, Ne(-1));
    buffer.resize(msgSize);
    EXPECT_THAT(mq_getattr(queueTest, &queueAttrs), Eq(0));
    EXPECT_THAT(queueAttrs.mq_curmsgs, Eq(10));

    usleep(50);
    for (int i=10; i>0 ; i--)
    {
        buffer.resize(queueAttrs.mq_msgsize);
        msgSize = mq_receive(queueTest, &buffer[0], queueAttrs.mq_msgsize, &prio);
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
    FileManipulationClassWriter writingToAFile;
    QueueSendFile myQueueSend;
    std::string data = "I expect these data will be send in the Queue.\n";
    std::string filename = "TmpFile.txt";
    writingToAFile.modifyBufferToWrite(data);
    ASSERT_NO_THROW(writingToAFile.openFile(filename)); 
    EXPECT_NO_THROW(writingToAFile.syncFileWithBuffer());
    writingToAFile.closeFile();

    EXPECT_NO_THROW(myQueueSend.openFile(filename));
    EXPECT_NO_THROW(myQueueSend.openIPC());
    EXPECT_NO_THROW(myQueueSend.syncFileWithBuffer());
    EXPECT_NO_THROW(myQueueSend.syncIPCAndBuffer());


    std::string queueName = "/" + myQueueSend.getName();
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
    msgSize = mq_receive(queueTest, &buffer[0], queueAttrs.mq_msgsize, &prio);
    ASSERT_THAT(msgSize, Eq(myQueueSend.getBufferSize()));
    buffer.resize(msgSize);
    EXPECT_THAT(std::string (buffer.begin(), buffer.end()), StrEq(data));

    mq_close(queueTest);
    mq_unlink(queueName.c_str());

    remove(filename.c_str());

}

TEST(SyncBuffAndQueue, ReadSendQueueComplexMessage)
{
    QueueSendFile myQueueSend;
    FileManipulationClassWriter writingToAFile;
    std::string fileinput = "input.dat";
    CreateRandomFile randomFile {fileinput,5, 1};
    ASSERT_THAT(checkIfFileExists(fileinput), IsTrue());
    std::string fileoutput = "output.dat";
    std::string queueName = "/" + myQueueSend.getName();
    mqd_t queueTest;
    struct mq_attr queueAttrs;
    std::vector<char> buffer;
    size_t msgSize;
    unsigned int prio = 5;
    ssize_t fileSize = returnFileSize(fileinput);
    ssize_t datasent = 0;

    ASSERT_NO_THROW(myQueueSend.openFile(fileinput));
    ASSERT_NO_THROW(myQueueSend.openIPC());
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
        msgSize = mq_receive(queueTest, &buffer[0], queueAttrs.mq_msgsize, &prio);
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

TEST(BasicQueueCmd, ReceiveQueueOpenCloseQueue)
{
    std::string queueName;
    mqd_t queueTest;
    {
        QueueReceiveFile myQueueObject;
        queueName = "/" + myQueueObject.getName();
        queueTest = mq_open(queueName.c_str(), O_CREAT | O_WRONLY,S_IRWXG |S_IRWXU,NULL);
        ASSERT_THAT(queueTest, Ne(-1));

        EXPECT_NO_THROW(myQueueObject.openIPC());

        mq_close(queueTest);
    }

    // the queue should be unlinked
    queueTest = mq_open(queueName.c_str(), O_RDONLY);
    EXPECT_THAT(queueTest, Eq(-1));
    EXPECT_THAT(errno, Eq(2)); //ENOENT

    {
        QueueTestReceiveFile myQueueObject;
        myQueueObject.changeMaxAttempt(1);
        CaptureStream stdcout{std::cout};
        EXPECT_THROW(myQueueObject.openIPC(),std::runtime_error);
        EXPECT_THAT(stdcout.str(),StrEq("Waiting to the ipcsendfile.\n"));
    }

    mq_close(queueTest);
}

TEST(SyncBuffAndQueue, ReceiveQueue)
{
    QueueTestReceiveFile myQueueObj;
    mqd_t queueTest;
    std::string queueName = "/" + myQueueObj.getName();
    struct mq_attr queueAttrs;
    queueAttrs.mq_msgsize = 4096;
    queueAttrs.mq_maxmsg = 10;

    //Trying to sync before opening
    EXPECT_THROW(myQueueObj.syncIPCAndBuffer(), std::runtime_error);


    //open
    queueTest = mq_open(queueName.c_str(), O_CREAT | O_WRONLY,S_IRWXG |S_IRWXU,&queueAttrs);
    ASSERT_THAT(queueTest, Ne(-1));
    EXPECT_NO_THROW(myQueueObj.openIPC());

    // Text message
    std::string message = "This is a test.";
    mq_send(queueTest, message.c_str(), message.size(), 5);
    EXPECT_NO_THROW(myQueueObj.syncIPCAndBuffer());
    std::vector<char> output = myQueueObj.getBuffer();
    EXPECT_THAT(std::string (output.begin(), output.end()), StrEq(message));

    //binary message
    ssize_t randomSize = rand() % 4096;
    std::vector<char> randomData = getRandomData(randomSize);
    mq_send(queueTest, &randomData[0], randomSize, 5);
    EXPECT_NO_THROW(myQueueObj.syncIPCAndBuffer());
    EXPECT_THAT(&myQueueObj.getBuffer()[0], StrEq(&randomData[0]));

    mq_close(queueTest);
}

TEST(SyncBuffandQueue, ReceiveQueueAndWrite)
{
    QueueReceiveFile myQueueObj;
    FileManipulationClassReader Reader;

    //files param
    std::string fileinput = "input.dat";
    CreateRandomFile randomFile {fileinput,5, 1};
    ASSERT_THAT(checkIfFileExists(fileinput), IsTrue());
    std::string fileoutput = "output.dat";

    //the sender param
    mqd_t queueTest;
    struct mq_attr queueAttrs;
    std::string queueName = "/" + myQueueObj.getName();
    queueAttrs.mq_maxmsg = 10;
    queueAttrs.mq_msgsize = 4096;
    std::vector<char> buffer;
    ssize_t fileSize = returnFileSize(fileinput);
    ssize_t datasent = 0;

    //Opening
    ASSERT_NO_THROW(myQueueObj.openFile(fileoutput));
    ASSERT_NO_THROW(Reader.openFile(fileinput));
    queueTest = mq_open(queueName.c_str(), O_CREAT | O_WRONLY, S_IRWXG |S_IRWXU,&queueAttrs);
    ASSERT_THAT(queueTest, Ne(-1));
    ASSERT_NO_THROW(myQueueObj.openIPC());

    //Loop
    while (datasent < fileSize)
    {
        std::vector<char> (4096).swap(buffer);
        ASSERT_NO_THROW(Reader.syncFileWithBuffer());
        buffer = Reader.getBufferRead();
        mq_send(queueTest, &buffer[0], buffer.size(), 5);
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
    ASSERT_NO_THROW(myQueueSend.openIPC());
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
    ASSERT_NO_THROW(myQueueReceive.openIPC());

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
    usleep(50);
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
    EXPECT_NO_THROW(myQueueSend.syncFileWithIPC("input.dat"));
}

void ThreadQueueReceiveAuto(void)
{
    QueueReceiveFile myQueueReceive;
    EXPECT_NO_THROW(myQueueReceive.syncFileWithIPC("output.dat"));
}
TEST(QueueSendAndReceive, UsingsyncFileWithIPC)
{
    std::string fileinput = "input.dat";
    std::string fileoutput = "output.dat";
    
    CreateRandomFile randomFile {fileinput,5, 1};

    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadQueueSendAuto);
    usleep(50);
    start_pthread(&mThreadID2,ThreadQueueReceiveAuto);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 


    EXPECT_THAT(compareFiles(fileoutput, fileinput), IsTrue());


    remove(fileoutput.c_str());
}

