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


TEST(ShmHandler, Constructors)
{
    HandyFunctions toolBox;

    std::string fileName = "myFileName";
    CreateRandomFile myFile(fileName,1,1);

    EXPECT_NO_THROW(sendShmHandler(&toolBox, "/myQueue", fileName));
    EXPECT_NO_THROW(receiveShmHandler(&toolBox, "/myQueue", "myFile"));

    remove("myFile");
}

TEST(ShmHandler, SenderConnectAlone)
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

    sendShmHandler mySender(&mockedTB, "myShm", fileName);

    ASSERT_THROW(mySender.connect(), ipc_exception);

}

TEST(ShmHandler, ReceiverConnectAlone)
{
    MockToolBox mockedTB;


    EXPECT_CALL(mockedTB,checkIfFileExists(A<const std::string&>()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mockedTB,getMaxAttempt())
        .WillOnce(Return(0));
    EXPECT_CALL(mockedTB,nap(A<int>()))
        .WillOnce(Return());
    EXPECT_CALL(mockedTB,updatePrintingElements(A<std::string>(), A<bool>()))
        .WillOnce(Return());

    receiveShmHandler myReceiver(&mockedTB, "myShm", "myFileName");
    
    ASSERT_THROW(myReceiver.connect(), ipc_exception);

    remove("myFileName");
}

TEST(ShmHandler, ConnectTogether)
{
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    std::string shmName = "/myShm";

    CreateRandomFile myFile(SenderfileName,1,1);
    HandyFunctions myToolBox;

    ASSERT_NO_THROW(
        {
            sendShmHandler Sender(&myToolBox, shmName, SenderfileName);
            receiveShmHandler Receiver(&myToolBox, shmName, ReceiverfileName);

            auto senderConnect = std::async(std::launch::async, [&](){Sender.connect();});
            usleep(50);
            auto receiverConnect = std::async(std::launch::async, [&](){Receiver.connect();});

            senderConnect.get();
            receiverConnect.get();
        }
    );

    
    remove(ReceiverfileName.c_str());
}


TEST(ShmHandler, SendReceiveHeader)
{
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    std::string shmName = "/myShm";

    CreateRandomFile myFile(SenderfileName,1,1);
    HandyFunctions myToolBox;
    size_t fileSize;

    ASSERT_NO_THROW(
        {
            sendShmHandler Sender(&myToolBox, shmName, SenderfileName);
            receiveShmHandler Receiver(&myToolBox, shmName, ReceiverfileName);

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


TEST(ShmHandler, TransferData)
{
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    std::string shmName = "/myShm";

    HandyFunctions myToolBox;

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
            sendShmHandler Sender(&myToolBox, shmName, SenderfileName);
            receiveShmHandler Receiver(&myToolBox, shmName, ReceiverfileName);

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


TEST(ShmHandler, copyFile)
{
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName2" ;

    HandyFunctions myToolBox1;
    HandyFunctions myToolBox2;
    
    CreateRandomFile myFile(SenderfileName, rand()%20+1, rand()%20+1);
    ASSERT_THAT(myToolBox1.checkIfFileExists(SenderfileName), IsTrue());

    std::vector<const char*> SenderArguments {"--shm", "--file", "myFileName"};
    std::vector<const char*> ReceiverArguments {"--shm", "--file", "myRFileName2"};
    FakeCmdLineOpt SenderFakeOpt(SenderArguments.begin(), SenderArguments.end());
    FakeCmdLineOpt ReceiverFakeOpt(ReceiverArguments.begin(), ReceiverArguments.end());
    
    ASSERT_NO_THROW(
        {
            copyFileThroughIPC Sender(SenderFakeOpt.argc(), SenderFakeOpt.argv(), &myToolBox1, program::SENDER);
            copyFileThroughIPC Receiver(ReceiverFakeOpt.argc(), ReceiverFakeOpt.argv(), &myToolBox2, program::RECEIVER);
            
            auto senderThread = std::async(std::launch::async, [&]()
            {
                Sender.launch();
            });
            usleep(50);
            auto receiverThread = std::async(std::launch::async, [&]()
            {
                Receiver.launch();
            });

            senderThread.get();
            receiverThread.get();
        });
    EXPECT_THAT(compareFiles(SenderfileName, ReceiverfileName), IsTrue());
    remove(ReceiverfileName.c_str());
}


TEST(ShmHandler, SenderCrashed)
{
    HandyFunctions myToolBox1;
    MockToolBoxAttempt myToolBox2;

    EXPECT_CALL(myToolBox2, getMaxAttempt())
        .WillRepeatedly(Return(1));
    
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName3";
    
    CreateRandomFile myFile(SenderfileName, rand()%20+1, rand()%20+1);
    ASSERT_THAT(myToolBox1.checkIfFileExists(SenderfileName), IsTrue());

    std::vector<const char*> ReceiverArguments {"--shm", "--file", "myRFileName3"};
    FakeCmdLineOpt ReceiverFakeOpt(ReceiverArguments.begin(), ReceiverArguments.end());
    std::vector<char> senderBuffer;

    ASSERT_THROW(
    {
        sendShmHandler Sender(&myToolBox1, "/ShmIPC", SenderfileName);
        copyFileThroughIPC Receiver(ReceiverFakeOpt.argc(), ReceiverFakeOpt.argv(), &myToolBox2, program::RECEIVER);
        auto senderThread = std::async(std::launch::async, [&]()
        {
            Sender.connect();
            Sender.transferHeader();
            int nbOfLoop = rand() %100+20;
            for (int i =0; i<nbOfLoop; i++)
            {
                Sender.transferData(senderBuffer);

            }
        });
        usleep(50);
        auto receiverThread = std::async(std::launch::async, [&]()
        {
            Receiver.launch();
        });

        senderThread.get();
        receiverThread.get();
    },
        ipc_exception
    );

    remove(ReceiverfileName.c_str());
}