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
using ::testing::_;
using ::testing::A;
using ::testing::Return;



TEST(QueueHandler, Constructors)
{
    handyFunctions toolBox;

    std::string fileName = "myFileName";
    CreateRandomFile myFile(fileName,1,1);

    EXPECT_NO_THROW(sendQueueHandler(&toolBox, "/myQueue", fileName));
    EXPECT_NO_THROW(receiveQueueHandler(&toolBox, "/myQueue", "myFile"));

    remove("myFile");
}


TEST(QueueHandler, SenderConnectAlone)
{
    MockToolBox mockedTB;


    std::string fileName = "myFileName";
    CreateRandomFile myFile(fileName,1,1);

    EXPECT_CALL(mockedTB,checkIfFileExists(A<const std::string&>()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mockedTB,returnFileSize(A<const std::string&>()))
        .WillRepeatedly(Return(1000));
    EXPECT_CALL(mockedTB, enoughSpaceAvailable(A<size_t>()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mockedTB,getMaxAttempt())
        .WillOnce(Return(0));

    
    sendQueueHandler mySender(&mockedTB, "/myQueue", fileName);

    ASSERT_THROW(mySender.connect(), ipc_exception);
}

TEST(QueueHandler, ReceiverConnectAlone)
{
    MockToolBox mockedTB;


    EXPECT_CALL(mockedTB,getMaxAttempt())
        .WillOnce(Return(0));
    EXPECT_CALL(mockedTB,checkIfFileExists(A<const std::string&>()))
        .WillRepeatedly(Return(false));

    
    receiveQueueHandler myReceiver(&mockedTB, "/myQueue", "myFileName");

    ASSERT_NO_THROW(myReceiver.connect());
    ASSERT_THROW(myReceiver.transferHeader(), ipc_exception);

    remove("myFileName");
}

TEST(QueueHandler, ConnectTogether)
{
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    std::string queueName = "/myQueue";

    CreateRandomFile myFile(SenderfileName,1,1);
    handyFunctions myToolBox;

    ASSERT_NO_THROW(
        {
            sendQueueHandler Sender(&myToolBox, queueName, SenderfileName);
            receiveQueueHandler Receiver(&myToolBox, queueName, ReceiverfileName);

            auto senderConnect = std::async(std::launch::async, [&](){Sender.connect();});
            usleep(50);
            auto receiverConnect = std::async(std::launch::async, [&](){Receiver.connect();});

            senderConnect.get();
            receiverConnect.get();
        }
    );


    remove(ReceiverfileName.c_str());
}


TEST(QueueHandler, SendandReceiveData)
{
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    std::string queueName = "/myQueue";

    CreateRandomFile myFile(SenderfileName,1,1);
    handyFunctions myToolBox;

    srand (time(NULL));
    std::vector<char> sendVector = getRandomData();
    std::vector<char> receiveVector;
    receiveVector.resize(myToolBox.getDefaultBufferSize());

    ASSERT_NO_THROW(
        {
            sendQueueHandler Sender(&myToolBox, queueName, SenderfileName);
            receiveQueueHandler Receiver(&myToolBox, queueName, ReceiverfileName);

            auto senderConnect = std::async(std::launch::async, [&](){
                Sender.connect();
                Sender.sendData(sendVector.data(), sendVector.size());
            });
            usleep(50);
            auto receiverConnect = std::async(std::launch::async, [&](){
                Receiver.connect();
                size_t bytesInPipe = Receiver.receiveData(receiveVector.data(), myToolBox.getDefaultBufferSize());
                receiveVector.resize(bytesInPipe);
            });

            senderConnect.get();
            receiverConnect.get();
        }
    );

    EXPECT_THAT(std::equal(sendVector.begin(),sendVector.end(), receiveVector.begin()), IsTrue());
    remove(ReceiverfileName.c_str());
}


TEST(QueueHandler, SendReceiveHeader)
{
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    std::string queueName = "/myQueue";

    CreateRandomFile myFile(SenderfileName,1,1);
    handyFunctions myToolBox;
    size_t fileSize;

    ASSERT_NO_THROW(
        {
            sendQueueHandler Sender(&myToolBox, queueName, SenderfileName);
            receiveQueueHandler Receiver(&myToolBox, queueName, ReceiverfileName);

            auto senderConnect = std::async(std::launch::async, [&](){
                Sender.connect();
                Sender.transferHeader();
            });
            usleep(50);
            auto receiverConnect = std::async(std::launch::async, [&](){
                Receiver.connect();
                fileSize = Receiver.transferHeader();
            });

            senderConnect.get();
            receiverConnect.get();
        }
    );

    EXPECT_THAT(fileSize,Eq(myToolBox.returnFileSize(SenderfileName)));
    remove(ReceiverfileName.c_str());

}

TEST(QueueHandler, TransferData)
{
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    std::string queueName = "/myQueue";

    handyFunctions myToolBox;

    //creating a file of less than 4096 bytes
    std::vector<char> sendVector = getRandomData();
    std::fstream tempFile(SenderfileName, std::ios::out | std::ios::binary);
    tempFile.write(sendVector.data(), sendVector.size());
    tempFile.close();
    ASSERT_THAT(myToolBox.checkIfFileExists(SenderfileName), IsTrue());

    size_t fileSize;
    std::vector<char> senderBuffer;
    std::vector<char> receiverBuffer;

    ASSERT_NO_THROW(
        {
            sendQueueHandler Sender(&myToolBox, queueName, SenderfileName);
            receiveQueueHandler Receiver(&myToolBox, queueName, ReceiverfileName);

            auto senderConnect = std::async(std::launch::async, [&](){
                Sender.connect();
                Sender.transferHeader();
                Sender.transferData(senderBuffer);
            });
            usleep(50);
            auto receiverConnect = std::async(std::launch::async, [&](){
                Receiver.connect();
                fileSize = Receiver.transferHeader();
                Receiver.transferData(receiverBuffer);
            });

            senderConnect.get();
            receiverConnect.get();
        }
    );

    EXPECT_THAT(compareFiles(SenderfileName, ReceiverfileName), IsTrue());
    remove(ReceiverfileName.c_str());
    remove(SenderfileName.c_str());
}

TEST(QueueHandler, copyFile)
{
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName" ;

    handyFunctions myToolBox1;
    handyFunctions myToolBox2;
    
    CreateRandomFile myFile(SenderfileName, rand()%20+1, rand()%20+1);
    ASSERT_THAT(myToolBox1.checkIfFileExists(SenderfileName), IsTrue());

    std::vector<const char*> SenderArguments {"--queue", "--file", "myFileName"};
    std::vector<const char*> ReceiverArguments {"--queue", "--file", "myRFileName"};
    FakeCmdLineOpt SenderFakeOpt(SenderArguments.begin(), SenderArguments.end());
    FakeCmdLineOpt ReceiverFakeOpt(ReceiverArguments.begin(), ReceiverArguments.end());
    
    ASSERT_NO_THROW(
        {
            auto senderThread = std::async(std::launch::async, [&]()
            {
                copyFileThroughIPC Sender(SenderFakeOpt.argc(), SenderFakeOpt.argv(), &myToolBox1, program::SENDER);
                Sender.launch();
            });
            usleep(50);
            auto receiverThread = std::async(std::launch::async, [&]()
            {
                copyFileThroughIPC Receiver(ReceiverFakeOpt.argc(), ReceiverFakeOpt.argv(), &myToolBox2, program::RECEIVER);
                Receiver.launch();
            });

            senderThread.get();
            receiverThread.get();
        });

    EXPECT_THAT(compareFiles(SenderfileName, ReceiverfileName), IsTrue());
    remove(ReceiverfileName.c_str());
}


TEST(QueueHandler, SenderCrashed)
{
    handyFunctions myToolBox1;
    MockToolBoxAttempt myToolBox2;

    EXPECT_CALL(myToolBox2, getMaxAttempt())
        .WillRepeatedly(Return(1));
    
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    
    CreateRandomFile myFile(SenderfileName, rand()%20+1, rand()%20+1);
    ASSERT_THAT(myToolBox1.checkIfFileExists(SenderfileName), IsTrue());

    std::vector<const char*> ReceiverArguments {"--queue", "--file", "myRFileName"};
    FakeCmdLineOpt ReceiverFakeOpt(ReceiverArguments.begin(), ReceiverArguments.end());
    std::vector<char> senderBuffer;

    ASSERT_THROW(
    {
        auto senderThread = std::async(std::launch::async, [&]()
        {
            sendQueueHandler Sender(&myToolBox1, "/QueueIPC", SenderfileName);
            Sender.connect();
            Sender.transferHeader();
            int nbOfLoop = rand() %20+1;
            for (int i =0; i<nbOfLoop; i++)
                Sender.transferData(senderBuffer);
        });
        usleep(50);
        auto receiverThread = std::async(std::launch::async, [&]()
        {
            copyFileThroughIPC Receiver(ReceiverFakeOpt.argc(), ReceiverFakeOpt.argv(), &myToolBox2, program::RECEIVER);
            Receiver.launch();
        });

        senderThread.get();
        receiverThread.get();
    },
        ipc_exception
    );

    remove(ReceiverfileName.c_str());
}


TEST(QueueHandler, ReceiverCrashed)
{
    MockToolBoxAttempt myToolBox1;
    handyFunctions myToolBox2;

    EXPECT_CALL(myToolBox1, getMaxAttempt())
        .WillRepeatedly(Return(1));

    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    
    CreateRandomFile myFile(SenderfileName, rand()%20+1, rand()%20+1);
    ASSERT_THAT(myToolBox1.checkIfFileExists(SenderfileName), IsTrue());

    std::vector<const char*> SenderArguments {"--queue", "--file", "myFileName"};
    FakeCmdLineOpt SenderFakeOpt(SenderArguments.begin(), SenderArguments.end());
    std::vector<char> ReceiverBuffer;

    ASSERT_THROW(
    {
        auto senderThread = std::async(std::launch::async, [&]()
        {
                copyFileThroughIPC Sender(SenderFakeOpt.argc(), SenderFakeOpt.argv(), &myToolBox1, program::SENDER);
                Sender.launch();
        });
        usleep(50);
        auto receiverThread = std::async(std::launch::async, [&]()
        {
            receiveQueueHandler Receiver(&myToolBox2, "/QueueIPC", ReceiverfileName);
            Receiver.connect();
            Receiver.transferHeader();
            int nbOfLoop = rand() %10+1;
            for (int i =0; i<nbOfLoop; i++)
                Receiver.transferData(ReceiverBuffer);
        });

        senderThread.get();
        receiverThread.get();
    },
        ipc_exception
    );

    remove(ReceiverfileName.c_str());
}



TEST(QueueHandler, DoubleSenders)
{
    MockToolBoxAttempt myToolBox1;
    MockToolBoxAttempt myToolBox2;
    MockToolBoxAttempt myToolBox3;

    EXPECT_CALL(myToolBox1, getMaxAttempt())
        .WillRepeatedly(Return(1));
    EXPECT_CALL(myToolBox2, getMaxAttempt())
        .WillRepeatedly(Return(1));
    EXPECT_CALL(myToolBox3, getMaxAttempt())
        .WillRepeatedly(Return(1));
    
    std::string SenderfileName = "myFileName" ;
    std::string ReceiverfileName = "myRFileName";
    
    CreateRandomFile myFile(SenderfileName, rand()%20+1, rand()%20+1);
    ASSERT_THAT(myToolBox1.checkIfFileExists(SenderfileName), IsTrue());

    std::vector<const char*> SenderArguments {"--queue", "--file", "myFileName"};
    std::vector<const char*> ReceiverArguments {"--queue", "--file", "myRFileName"};
    FakeCmdLineOpt SenderFakeOpt(SenderArguments.begin(), SenderArguments.end());
    FakeCmdLineOpt ReceiverFakeOpt(ReceiverArguments.begin(), ReceiverArguments.end());
    
    ASSERT_THROW(
        {
            auto sender1Thread = std::async(std::launch::async, [&]()
            {
                copyFileThroughIPC Sender(SenderFakeOpt.argc(), SenderFakeOpt.argv(), &myToolBox1, program::SENDER);
                Sender.launch();
            });
            usleep(50);
            auto sender2Thread = std::async(std::launch::async, [&]()
            {
                copyFileThroughIPC Sender(SenderFakeOpt.argc(), SenderFakeOpt.argv(), &myToolBox3, program::SENDER);
                Sender.launch();
            });

            usleep(1000);
            auto receiverThread = std::async(std::launch::async, [&]()
            {
                copyFileThroughIPC Receiver(ReceiverFakeOpt.argc(), ReceiverFakeOpt.argv(), &myToolBox2, program::RECEIVER);
                Receiver.launch();
            });

            sender1Thread.get();
            sender2Thread.get();
            receiverThread.get();
        },
        ipc_exception
        );

    remove(ReceiverfileName.c_str());
}



TEST(QueueHandler, DoubleReceivers)
{
    MockToolBoxAttempt myToolBox1;
    MockToolBoxAttempt myToolBox2;
    MockToolBoxAttempt myToolBox3;

    EXPECT_CALL(myToolBox1, getMaxAttempt())
        .WillRepeatedly(Return(1));
    EXPECT_CALL(myToolBox2, getMaxAttempt())
        .WillRepeatedly(Return(1));
    EXPECT_CALL(myToolBox3, getMaxAttempt())
        .WillRepeatedly(Return(1));
    
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    
    CreateRandomFile myFile(SenderfileName, rand()%20+1, rand()%20+1);
    ASSERT_THAT(myToolBox1.checkIfFileExists(SenderfileName), IsTrue());

    std::vector<const char*> SenderArguments {"--queue", "--file", "myFileName"};
    std::vector<const char*> ReceiverArguments {"--queue", "--file", "myRFileName"};
    FakeCmdLineOpt SenderFakeOpt(SenderArguments.begin(), SenderArguments.end());
    FakeCmdLineOpt ReceiverFakeOpt(ReceiverArguments.begin(), ReceiverArguments.end());
    
    ASSERT_THROW(
        {
            auto sender1Thread = std::async(std::launch::async, [&]()
            {
                copyFileThroughIPC Sender(SenderFakeOpt.argc(), SenderFakeOpt.argv(), &myToolBox1, program::SENDER);
                Sender.launch();
            });
            
            usleep(1000);
            auto receiver2Thread = std::async(std::launch::async, [&]()
            {
                copyFileThroughIPC Receiver(ReceiverFakeOpt.argc(), ReceiverFakeOpt.argv(), &myToolBox3, program::RECEIVER);
                Receiver.launch();
            });

            usleep(1000);
            auto receiverThread = std::async(std::launch::async, [&]()
            {
                copyFileThroughIPC Receiver(ReceiverFakeOpt.argc(), ReceiverFakeOpt.argv(), &myToolBox2, program::RECEIVER);
                Receiver.launch();
            });

            sender1Thread.get();
            receiver2Thread.get();
            receiverThread.get();
        },
        ipc_exception
        );

    remove(ReceiverfileName.c_str());
}
