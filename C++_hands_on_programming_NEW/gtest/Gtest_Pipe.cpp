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


TEST(FifoHandler, ConstructorAndDestructor)
{
    HandyFunctions myToolBox;
    {
        FifoHandler myFifo(&myToolBox, "HelloFifo");
        myFifo.createFifo();
        EXPECT_THAT(myToolBox.checkIfFileExists("HelloFifo"), IsTrue());
    }
    EXPECT_THAT(myToolBox.checkIfFileExists("HelloFifo"), IsFalse());
}

TEST(PipeHandler, ConstructorSender)
{
    std::string fileName = "myFileName";
    CreateRandomFile myFile(fileName,1,1);
    HandyFunctions myToolBox;
    
    EXPECT_THROW(SendPipeHandler(&myToolBox,"myPipe","FileDoesNotExists"),file_exception);

    ASSERT_NO_THROW(SendPipeHandler(&myToolBox,"myPipe",fileName));
    EXPECT_THAT(myToolBox.checkIfFileExists("myPipe"), IsFalse());
}

TEST(PipeHandler, ConstructorSenderSignalHandler)
{
    std::string fileName = "myFileName";
    CreateRandomFile myFile(fileName,1,1);
    HandyFunctions myToolBox;

    SendPipeHandler myPipeHandler(&myToolBox,"myPipe",fileName);
    ASSERT_THAT(sigpipe_received, IsFalse());
    raise(SIGPIPE);
    EXPECT_THAT(sigpipe_received, IsTrue()); 
}

TEST(PipeHandler, ConstructorReceiver)
{
    std::string fileName = "myFileName";
    MockToolBox mockedTB;

    EXPECT_CALL(mockedTB,checkIfFileExists(A<const std::string&>()))
        .WillOnce(Return(false))
        .WillOnce(Return(true))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(mockedTB,getMaxAttempt())
        .WillOnce(Return(10))
        .WillOnce(Return(0));

    ASSERT_NO_THROW(ReceivePipeHandler(&mockedTB, "myPipe", fileName)); //mocked the pipe exists
    ASSERT_THROW(ReceivePipeHandler(&mockedTB, "myPipe", fileName), ipc_exception); //mocked the pipe doesn't exists

    remove(fileName.c_str());  
}

TEST(PipeHandler, SenderConnectAlone)
{
    std::string fileName = "myFileName";
    CreateRandomFile myFile(fileName,1,1);
    MockToolBox mockedTB;

    EXPECT_CALL(mockedTB,checkIfFileExists(A<const std::string&>()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mockedTB,returnFileSize(A<const std::string&>()))
        .WillRepeatedly(Return(1000));
    EXPECT_CALL(mockedTB, enoughSpaceAvailable(A<size_t>()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mockedTB,getMaxAttempt())
        .WillOnce(Return(0));
    
    SendPipeHandler myPipeHandler(&mockedTB,"myPipe",fileName);
    ASSERT_THROW(myPipeHandler.connect(), ipc_exception);
}

TEST(PipeHandler, ConnectTogether)
{
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    std::string pipeName = "myPipe";

    CreateRandomFile myFile(SenderfileName,1,1);
    HandyFunctions myToolBox;

    ASSERT_NO_THROW(
        {
            SendPipeHandler Sender(&myToolBox, pipeName, SenderfileName);
            ReceivePipeHandler Receiver(&myToolBox, pipeName, ReceiverfileName);

            auto senderConnect = std::async(std::launch::async, [&](){Sender.connect();});
            usleep(50);
            auto receiverConnect = std::async(std::launch::async, [&](){Receiver.connect();});

            senderConnect.get();
            receiverConnect.get();
        }
    );

    remove(ReceiverfileName.c_str());
}

TEST(PipeHandler, SendandReceiveData)
{
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    std::string pipeName = "myPipe";

    CreateRandomFile myFile(SenderfileName,1,1);
    HandyFunctions myToolBox;
    srand (time(NULL));
    std::vector<char> sendVector = getRandomData();
    std::vector<char> receiveVector;
    receiveVector.resize(myToolBox.getDefaultBufferSize());

    ASSERT_NO_THROW(
        {
            auto senderConnect = std::async(std::launch::async, [&]()
            {
                SendPipeHandler Sender(&myToolBox, pipeName, SenderfileName);
                Sender.connect();
                Sender.sendData(sendVector.data(), sendVector.size());
            });
            auto receiverConnect = std::async(std::launch::async, [&]()
            {
                ReceivePipeHandler Receiver(&myToolBox, pipeName, ReceiverfileName);
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

TEST(PipeHandler, SendReceiveHeader)
{
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    std::string pipeName = "myPipe";

    CreateRandomFile myFile(SenderfileName,1,1);
    HandyFunctions myToolBox;
    size_t fileSize;

    ASSERT_NO_THROW(
        {
            auto senderConnect = std::async(std::launch::async, [&]()
            {
                SendPipeHandler Sender(&myToolBox, pipeName, SenderfileName);
                Sender.connect();
                Sender.transferHeader();
            });
            auto receiverConnect = std::async(std::launch::async, [&]()
            {
                ReceivePipeHandler Receiver(&myToolBox, pipeName, ReceiverfileName);
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

TEST(PipeHandler, TransferData)
{
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    std::string pipeName = "myPipe";

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
            auto senderConnect = std::async(std::launch::async, [&]()
            {
                SendPipeHandler Sender(&myToolBox, pipeName, SenderfileName);
                Sender.connect();
                Sender.transferHeader();
                Sender.transferData(senderBuffer);
            });
            auto receiverConnect = std::async(std::launch::async, [&]()
            {
                ReceivePipeHandler Receiver(&myToolBox, pipeName, ReceiverfileName);
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

TEST(PipeHandler, copyFile)
{
    HandyFunctions myToolBox1;
    HandyFunctions myToolBox2;
    
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName" ;
    
    CreateRandomFile myFile(SenderfileName, rand()%20+1, rand()%20+1);
    ASSERT_THAT(myToolBox1.checkIfFileExists(SenderfileName), IsTrue());

    std::vector<const char*> SenderArguments {"--pipe", "--file", "myFileName"};
    std::vector<const char*> ReceiverArguments {"--pipe", "--file", "myRFileName"};
    FakeCmdLineOpt SenderFakeOpt(SenderArguments.begin(), SenderArguments.end());
    FakeCmdLineOpt ReceiverFakeOpt(ReceiverArguments.begin(), ReceiverArguments.end());
    
    ASSERT_NO_THROW(
        {
            auto senderThread = std::async(std::launch::async, [&]()
            {
                CopyFileThroughIPC Sender(SenderFakeOpt.argc(), SenderFakeOpt.argv(), &myToolBox1, program::SENDER);
                Sender.launch();
            });
            usleep(50);
            auto receiverThread = std::async(std::launch::async, [&]()
            {
                CopyFileThroughIPC Receiver(ReceiverFakeOpt.argc(), ReceiverFakeOpt.argv(), &myToolBox2, program::RECEIVER);
                Receiver.launch();
            });

            senderThread.get();
            receiverThread.get();
        });

    EXPECT_THAT(compareFiles(SenderfileName, ReceiverfileName), IsTrue());
    remove(ReceiverfileName.c_str());
}

TEST(PipeHandler, SenderCrashed)
{
    HandyFunctions myToolBox1;
    HandyFunctions myToolBox2;
    
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    
    CreateRandomFile myFile(SenderfileName, rand()%20+1, rand()%20+1);
    ASSERT_THAT(myToolBox1.checkIfFileExists(SenderfileName), IsTrue());

    std::vector<const char*> ReceiverArguments {"--pipe", "--file", "myRFileName"};
    FakeCmdLineOpt ReceiverFakeOpt(ReceiverArguments.begin(), ReceiverArguments.end());
    std::vector<char> senderBuffer;

    ASSERT_THROW(
    {
        auto senderThread = std::async(std::launch::async, [&]()
        {
            SendPipeHandler Sender(&myToolBox1, "PipeIPC", SenderfileName);
            Sender.connect();
            Sender.transferHeader();
            int nbOfLoop = rand() %20+1;
            for (int i =0; i<nbOfLoop; i++)
                Sender.transferData(senderBuffer);
        });
        usleep(50);
        auto receiverThread = std::async(std::launch::async, [&]()
        {
            CopyFileThroughIPC Receiver(ReceiverFakeOpt.argc(), ReceiverFakeOpt.argv(), &myToolBox2, program::RECEIVER);
            Receiver.launch();
        });

        senderThread.get();
        receiverThread.get();
    },
        ipc_exception
    );

    remove(ReceiverfileName.c_str());
}


TEST(PipeHandler, SenderCrashedJustBeforeOpeningPipe)
{
    HandyFunctions myToolBox1;
    MockToolBox mockedTB;
    
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    
    CreateRandomFile myFile(SenderfileName, rand()%20+1, rand()%20+1);
    ASSERT_THAT(myToolBox1.checkIfFileExists(SenderfileName), IsTrue());

    std::vector<const char*> ReceiverArguments {"--pipe", "--file", "myRFileName"};
    FakeCmdLineOpt ReceiverFakeOpt(ReceiverArguments.begin(), ReceiverArguments.end());
    std::vector<char> senderBuffer;

    EXPECT_CALL(mockedTB,checkIfFileExists(A<const std::string&>()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mockedTB,returnFileSize(A<const std::string&>()))
        .WillRepeatedly(Return(1000));
    EXPECT_CALL(mockedTB, enoughSpaceAvailable(A<size_t>()))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(mockedTB,getMaxAttempt())
        .WillOnce(Return(5))
        .WillOnce(Return(0));

    ASSERT_THROW(
    {
        auto senderThread = std::async(std::launch::async, [&]()
        {
            SendPipeHandler Sender(&myToolBox1, "PipeIPC", SenderfileName);
        });
        usleep(50);
        auto receiverThread = std::async(std::launch::async, [&]()
        {
            CopyFileThroughIPC Receiver(ReceiverFakeOpt.argc(), ReceiverFakeOpt.argv(), &mockedTB, program::RECEIVER);
            Receiver.launch();
        });

        senderThread.get();
        receiverThread.get();
    },
        ipc_exception
    );

    remove(ReceiverfileName.c_str());
}

TEST(PipeHandler, ReceiverCrashed)
{
    HandyFunctions myToolBox1;
    HandyFunctions myToolBox2;
    
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    
    CreateRandomFile myFile(SenderfileName, rand()%20+1, rand()%20+1);
    ASSERT_THAT(myToolBox1.checkIfFileExists(SenderfileName), IsTrue());

    std::vector<const char*> SenderArguments {"--pipe", "--file", "myFileName"};
    FakeCmdLineOpt SenderFakeOpt(SenderArguments.begin(), SenderArguments.end());
    std::vector<char> ReceiverBuffer;

    ASSERT_THROW(
    {
        auto senderThread = std::async(std::launch::async, [&]()
        {
                CopyFileThroughIPC Sender(SenderFakeOpt.argc(), SenderFakeOpt.argv(), &myToolBox1, program::SENDER);
                Sender.launch();
        });
        usleep(50);
        auto receiverThread = std::async(std::launch::async, [&]()
        {
            ReceivePipeHandler Receiver(&myToolBox2, "PipeIPC", ReceiverfileName);
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


TEST(PipeHandler, DoubleSenders)
{
    HandyFunctions myToolBox1;
    HandyFunctions myToolBox2;
    HandyFunctions myToolBox3;
    
    std::string SenderfileName = "myFileName" ;
    std::string ReceiverfileName = "myRFileName";
    
    CreateRandomFile myFile(SenderfileName, rand()%20+1, rand()%20+1);
    ASSERT_THAT(myToolBox1.checkIfFileExists(SenderfileName), IsTrue());

    std::vector<const char*> SenderArguments {"--pipe", "--file", "myFileName"};
    std::vector<const char*> ReceiverArguments {"--pipe", "--file", "myRFileName"};
    FakeCmdLineOpt SenderFakeOpt(SenderArguments.begin(), SenderArguments.end());
    FakeCmdLineOpt ReceiverFakeOpt(ReceiverArguments.begin(), ReceiverArguments.end());
    
    ASSERT_THROW(
        {
            auto sender1Thread = std::async(std::launch::async, [&]()
            {
                CopyFileThroughIPC Sender(SenderFakeOpt.argc(), SenderFakeOpt.argv(), &myToolBox1, program::SENDER);
                Sender.launch();
            });
            usleep(50);
            auto sender2Thread = std::async(std::launch::async, [&]()
            {
                CopyFileThroughIPC Sender(SenderFakeOpt.argc(), SenderFakeOpt.argv(), &myToolBox3, program::SENDER);
                Sender.launch();
            });

            usleep(1000);
            auto receiverThread = std::async(std::launch::async, [&]()
            {
                CopyFileThroughIPC Receiver(ReceiverFakeOpt.argc(), ReceiverFakeOpt.argv(), &myToolBox2, program::RECEIVER);
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


TEST(PipeHandler, DoubleReceivers)
{
    HandyFunctions myToolBox1;
    HandyFunctions myToolBox2;
    HandyFunctions myToolBox3;
    
    std::string SenderfileName = "myFileName";
    std::string ReceiverfileName = "myRFileName";
    
    CreateRandomFile myFile(SenderfileName, rand()%20+1, rand()%20+1);
    ASSERT_THAT(myToolBox1.checkIfFileExists(SenderfileName), IsTrue());

    std::vector<const char*> SenderArguments {"--pipe", "--file", "myFileName"};
    std::vector<const char*> ReceiverArguments {"--pipe", "--file", "myRFileName"};
    FakeCmdLineOpt SenderFakeOpt(SenderArguments.begin(), SenderArguments.end());
    FakeCmdLineOpt ReceiverFakeOpt(ReceiverArguments.begin(), ReceiverArguments.end());
    
    ASSERT_THROW(
        {
            auto sender1Thread = std::async(std::launch::async, [&]()
            {
                CopyFileThroughIPC Sender(SenderFakeOpt.argc(), SenderFakeOpt.argv(), &myToolBox1, program::SENDER);
                Sender.launch();
            });
            
            usleep(1000);
            auto receiver2Thread = std::async(std::launch::async, [&]()
            {
                CopyFileThroughIPC Receiver(ReceiverFakeOpt.argc(), ReceiverFakeOpt.argv(), &myToolBox3, program::RECEIVER);
                Receiver.launch();
            });

            usleep(1000);
            auto receiverThread = std::async(std::launch::async, [&]()
            {
                CopyFileThroughIPC Receiver(ReceiverFakeOpt.argc(), ReceiverFakeOpt.argv(), &myToolBox2, program::RECEIVER);
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