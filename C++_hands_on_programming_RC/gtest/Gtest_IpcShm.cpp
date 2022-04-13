#include <gtest/gtest.h>
#include <gmock/gmock.h>
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
#include <semaphore.h>
#include <sched.h>
#include <cstring>
#include "../src/lib/IpcCopyFile.h"
#include "../src/lib/IpcShm.h"

using ::testing::Eq;
using ::testing::Ne;
using ::testing::Lt;
using ::testing::StrEq;
using ::testing::IsTrue;
using ::testing::IsFalse;
using ::testing::StartsWith;

const std::string ipcName = "CopyDataThroughSharedMemory";
const std::string semRName =  "myReceiverSemaphore";
const std::string semSName =  "mySenderSemaphore";
std::string shmfileName1 = "shminput.dat";
std::string shmfileName2 = "shmoutput.dat";
std::vector<char> ShmrandomData = getRandomData();

class IpcShmSendFileTest : public ShmSendFile
{
    public:
        char* get_buffer() {return shm_.data;};

        void MokeSync(const std::string &filepath)
        {
            struct timespec ts;
            openFile(filepath);
            //wait for the receiver to connect
            if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
            {
                throw ipc_exception("Error getting time");
            }
            ts.tv_sec += maxAttempt_;
            if (sem_timedwait(senderSemaphorePtr_,&ts) == -1) 
            {
                throw ipc_exception("Error, can't connect to the other program.\n");
            }
            //sending header
            Header header(filepath, defaultBufferSize_);
            std::memcpy(shm_.data, header.getHeader().data(), defaultBufferSize_);
            shm_.main->data_size = defaultBufferSize_;

            if(sem_post(receiverSemaphorePtr_) == -1)
            {
                throw ipc_exception(
                    "ShmSendFile::syncFileWithIPC(). Error when waiting the semaphore. Errno"
                    + std::string(strerror(errno))
                );
            }
            srand (time(NULL));
            int timeSyncsBeforeStopping = rand() % 20 +1;
            int timesync = 0;
            while (timesync++ < timeSyncsBeforeStopping)
            {
                if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
                {
                    throw ipc_exception("Error getting time");
                }
                ts.tv_sec += maxAttempt_;
                if (sem_timedwait(senderSemaphorePtr_,&ts) == -1)
                {
                    if (errno == ETIMEDOUT)
                        throw ipc_exception("Error. Can't find the other program. Did it crash ?\n");
                        
                    throw ipc_exception(
                        "ShmSendFile::syncFileWithIPC(). Error when waiting the semaphore. Errno"
                        + std::string(strerror(errno))
                    );
                }
                file_.read(shm_.data,bufferSize_);
                bufferSize_ = file_.gcount();
                shm_.main->data_size = bufferSize_;
                if(sem_post(receiverSemaphorePtr_) == -1)
                {
                    throw ipc_exception(
                        "ShmSendFile::syncFileWithIPC(). Error when waiting the semaphore. Errno"
                        + std::string(strerror(errno))
                    );
                }
            }
        }
};

class IpcShmReceiveFileTest : public ShmReceiveFile
{
    public:
        void setBufferSize(size_t size)
        {
            bufferSize_ = size;
        }
        void MokeSync()
        {
            struct timespec ts;
            sem_post(senderSemaphorePtr_); //letting the sender send some data

            //receiving header
            if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
            {
                throw ipc_exception("Error getting time");
            }
            ts.tv_sec += maxAttempt_;
            if(sem_timedwait(receiverSemaphorePtr_, &ts)==-1)
            {
                if (errno == ETIMEDOUT)
                    throw ipc_exception("Error. Can't find ipc_sendfile. Did it crash ?\n");

                throw ipc_exception(
                    "ShmReceiveFile::syncFileWithIPC(). Error when waiting for the semaphore. Errno: "
                    + std::string(strerror(errno))
                    );
            }

            Header header(defaultBufferSize_);
            std::vector<size_t> headerReceived;
            headerReceived.resize(defaultBufferSize_);
            std::memcpy(headerReceived.data(),shm_.data,defaultBufferSize_);
            if (header.getHeader()[0] != headerReceived[0])
            {
                throw ipc_exception("Error. Another message is present. Maybe another program uses this IPC.\n");
            }
            fileSize_ = headerReceived[1];

            sem_post(senderSemaphorePtr_);

            srand (time(NULL));
            int timeSyncsBeforeStopping = rand() % 20 +1;
            int timesync = 0;
            while (timesync++ < timeSyncsBeforeStopping)
            {
                if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
                {
                    throw ipc_exception("Error getting time");
                }
                ts.tv_sec += 1;
                if(sem_timedwait(receiverSemaphorePtr_, &ts)==-1)
                {
                    if (errno == ETIMEDOUT)
                        throw ipc_exception("Error. Can't find the other program. Did it crash ?\n");

                    throw ipc_exception(
                        "ShmReceiveFile::syncFileWithIPC(). Error when waiting for the semaphore. Errno: "
                        + std::string(strerror(errno))
                        );
                }
                sem_post(senderSemaphorePtr_);
            }

        }
};


////////////// ShmSendFile Constructor and Destructor ///////////////////
TEST(IpcShmSendFile, ConstructorDestructor)
{
    ASSERT_NO_THROW(ShmSendFile myShmObject);
    {
        ShmSendFile myShmObject;
        EXPECT_THAT(shm_open(ipcName.c_str(), O_RDWR,0), Ne(-1));
        EXPECT_THAT(sem_open(semSName.c_str(), 0), Ne(SEM_FAILED));
        EXPECT_THAT(sem_open(semRName.c_str(), 0), Ne(SEM_FAILED));
        shm_unlink(ipcName.c_str());
    }
    EXPECT_THAT(shm_open(ipcName.c_str(), O_RDWR,0), Eq(-1));
    EXPECT_THAT(sem_open(semSName.c_str(), 0), Eq(SEM_FAILED));
    EXPECT_THAT(sem_open(semRName.c_str(), 0), Eq(SEM_FAILED));
}

////////////// ShmSendFile launchedalone ///////////////////
TEST(NoOtherProgram, ShmSendFile)
{ 
    EXPECT_THROW({
        ShmSendFile myShmObject(1);
        CreateRandomFile myFile("input.dat", 1, 1);
        myShmObject.syncFileWithIPC("input.dat");
        }, ipc_exception);
    
}

////////////// ShmSendFile syncFileWithBuffer///////////////////
TEST(IpcShmSendFile, syncFileWithBuffer)
{
    {
        int shmFd = shm_open(ipcName.c_str(), O_RDONLY,0);
        sem_t* semaphorePtr = sem_open(semSName.c_str(), 0);
        ASSERT_THAT(shmFd, Eq(-1));
        ASSERT_THAT(semaphorePtr, Eq(SEM_FAILED));
        IpcShmSendFileTest myShmObject;

        //creating the file
        ASSERT_NO_THROW(
            {
                FileManipulationClassWriter writingToAFile;
                writingToAFile.modifyBufferToWrite(ShmrandomData);
                writingToAFile.openFile(shmfileName1.c_str());
                writingToAFile.syncFileWithBuffer();
            }
        );

        ASSERT_NO_THROW(myShmObject.openFile(shmfileName1));
        ASSERT_NO_THROW(myShmObject.syncFileWithBuffer(myShmObject.get_buffer()));

        EXPECT_TRUE(std::equal(ShmrandomData.begin(), ShmrandomData.end(), myShmObject.get_buffer()));

        remove(shmfileName1.c_str());
        close(shmFd);
        sem_close(semaphorePtr);
        shm_unlink(ipcName.c_str());
    }
    ASSERT_THAT(shm_open(ipcName.c_str(), O_RDWR,0), Eq(-1));
    ASSERT_THAT(sem_open(semSName.c_str(), 0), Eq(SEM_FAILED));
    ASSERT_THAT(sem_open(semRName.c_str(), 0), Eq(SEM_FAILED));
}


//////////////////// ShmSendFile syncFileWithIPC/////////////////
void IpcShmSendFilesyncFileWithIPC(void)
{
    ShmSendFile myShmObject;
    myShmObject.syncFileWithIPC(shmfileName1);

}

void IpcShmSendFilesyncFileWithIPC2(void)
{
    int fd = -1;
    sem_t* senderSemaphorePtr;
    sem_t* receiverSemaphorePtr;
    size_t size = 0;
    int wait = 0;
    FileManipulationClassReader GettingSomeInfo;


    do
    {
	    senderSemaphorePtr = sem_open(semSName.c_str(), O_RDWR);
    }
	while (senderSemaphorePtr == SEM_FAILED);
    do
    {
	    receiverSemaphorePtr = sem_open(semRName.c_str(), O_RDWR);
    }
	while (receiverSemaphorePtr == SEM_FAILED);

    fd = shm_open(ipcName.c_str(), O_RDWR, 0);
    
    ASSERT_THAT(fd, Ne(-1));

    size_t sizemap = sizeof(ShmData_Header) + GettingSomeInfo.getDefaultBufferSize();
    
    void* bufferPtr= mmap(NULL, sizemap, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (bufferPtr == MAP_FAILED)
    {
        ASSERT_THAT(1, Ne(2));
    }
    ShmData shmem;
    shmem.main = reinterpret_cast<ShmData_Header*>(bufferPtr);

    close(fd);
    while (!shmem.main->init_flag)
    {
        usleep(100);
        if (wait++ >10)
            return;
    }

    shmem.data = static_cast<char *>(static_cast<char *>(bufferPtr)+sizeof(ShmData_Header));

    fd = open(shmfileName2.c_str(), O_WRONLY | O_CREAT, 0660, NULL);
    

    sem_post(senderSemaphorePtr);

    //simulate receiving header
    sem_wait(receiverSemaphorePtr);
    sem_post(senderSemaphorePtr);

    usleep(10);
    do
    {
        sem_wait(receiverSemaphorePtr);

        write(fd, shmem.data, shmem.main->data_size);
        size = shmem.main->data_size;
        sem_post(senderSemaphorePtr);
        sched_yield();
    }
    while (size > 0);
    sem_close(receiverSemaphorePtr);
    sem_close(senderSemaphorePtr);
    munmap(bufferPtr, sizemap);
    close(fd);
    shm_unlink(ipcName.c_str());
}

TEST(IpcShmSendFile, syncFileWithIPC)
{
    ASSERT_THAT(shm_open(ipcName.c_str(), O_RDWR,0), Eq(-1));
    ASSERT_THAT(sem_open(semSName.c_str(), 0), Eq(SEM_FAILED));
    ASSERT_THAT(sem_open(semRName.c_str(), 0), Eq(SEM_FAILED));
    
    CreateRandomFile myRandomfile(shmfileName1,2,1);

    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,IpcShmSendFilesyncFileWithIPC);
    start_pthread(&mThreadID2,IpcShmSendFilesyncFileWithIPC2);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr);

    ASSERT_THAT(compareFiles(shmfileName1,shmfileName2), IsTrue);

    remove(shmfileName1.c_str());
    remove(shmfileName2.c_str());
}

///////////////// ShmReceivefile Constructor and Destructor ///////////////
TEST(IpcShmReceiveFile, ConstructorDestructor)
{
    ASSERT_THAT(shm_open(ipcName.c_str(), O_RDWR,0), Eq(-1));
    ASSERT_THAT(sem_open(semSName.c_str(), 0), Eq(SEM_FAILED));
    ASSERT_THAT(sem_open(semRName.c_str(), 0), Eq(SEM_FAILED));
    {
        ShmSendFile myShmSendObject;
        ASSERT_NO_THROW(ShmReceiveFile myShmReceiveObject);
        EXPECT_THAT(shm_open(ipcName.c_str(), O_RDWR,0), Eq(-1));
    }
    EXPECT_THAT(sem_open(semSName.c_str(), 0), Eq(SEM_FAILED));

    {
        CaptureStream stdcout(std::cout);
        ASSERT_THROW(ShmReceiveFile myShmReceiveObject(1), ipc_exception);
        EXPECT_THAT(stdcout.str(), StartsWith("Waiting for ipc_sendfile.\n"));
    }

    {
        sem_t* fdPtr = sem_open(semSName.c_str(), O_CREAT , S_IRWXU | S_IRWXG, 0);
        sem_t* fdPtr2 = sem_open(semRName.c_str(), O_CREAT , S_IRWXU | S_IRWXG, 0);
        ASSERT_THAT(fdPtr,Ne(SEM_FAILED));
        ASSERT_THROW(ShmReceiveFile myShmReceiveObject, ipc_exception);
        sem_close(fdPtr);
        sem_unlink(semSName.c_str());
        sem_close(fdPtr2);
        sem_unlink(semRName.c_str());
    }
}

////////////// ShmReceivefile launchedalone ///////////////////
TEST(NoOtherProgram, ShmReceivefile)
{ 
    EXPECT_THROW({
        ShmReceiveFile myShmObject(1);
        CreateRandomFile myFile("input.dat", 1, 1);
        myShmObject.syncFileWithIPC("input.dat");
        }, ipc_exception);
    
}
/////////////////////////// ShmReceivefile syncFileWithBuffer ///////////
TEST(IpcShmReceiveFile,syncFileWithBuffer)
{
    std::vector<char> someRandomData = getRandomData();
    {
        EXPECT_THAT(shm_open(ipcName.c_str(), O_RDWR,0), Eq(-1));
        EXPECT_THAT(sem_open(semSName.c_str(), 0), Eq(SEM_FAILED));
        EXPECT_THAT(sem_open(semRName.c_str(), 0), Eq(SEM_FAILED));
        ShmSendFile myShmSendObject;
        IpcShmReceiveFileTest myShmReceiveObject;
        myShmReceiveObject.openFile(shmfileName2);
        myShmReceiveObject.setBufferSize(someRandomData.size());
        myShmReceiveObject.syncFileWithBuffer(someRandomData.data());
    }

    int fd = open(shmfileName2.c_str(), O_RDONLY);
    ASSERT_THAT(fd,Ne(-1));


    std::vector<char> bufferForReading;
    bufferForReading.resize(someRandomData.size());
    ASSERT_THAT(bufferForReading.size(),Eq(someRandomData.size()));
    read(fd, bufferForReading.data(), bufferForReading.size());

    EXPECT_THAT(bufferForReading, Eq(someRandomData));
    remove(shmfileName2.c_str());
}


////////////////////////// ShmReceivefile and ShmSendfile /////////
void ThreadSendFile(void)
{

    ShmSendFile myShmSendObject1;
    myShmSendObject1.syncFileWithIPC(shmfileName1);
    
}

void ThreadReceiveFile(void)
{
    
    ShmReceiveFile myShmReceiveObject1;
    myShmReceiveObject1.syncFileWithIPC(shmfileName2);
    
}

TEST(ShmReceivefileAndShmSendfile, copyfileSendFileFirst)
{
    CaptureStream stdout(std::cout);
    CreateRandomFile myRandomfile(shmfileName1,2,2);

    pthread_t mThreadID1, mThreadID2;
    ASSERT_THAT(shm_open(ipcName.c_str(), O_RDWR,0), Eq(-1));
    ASSERT_THAT(sem_open(semSName.c_str(), 0), Eq(SEM_FAILED));
    ASSERT_THAT(sem_open(semRName.c_str(), 0), Eq(SEM_FAILED));

    start_pthread(&mThreadID1,ThreadSendFile);
    usleep(10);
    start_pthread(&mThreadID2,ThreadReceiveFile);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr);

    ASSERT_THAT(compareFiles(shmfileName1,shmfileName2), IsTrue);

    remove(shmfileName1.c_str());
    remove(shmfileName2.c_str());

    EXPECT_THAT(shm_open(ipcName.c_str(), O_RDWR,0), Eq(-1));
    EXPECT_THAT(sem_open(semSName.c_str(), 0), Eq(SEM_FAILED));
    EXPECT_THAT(sem_open(semRName.c_str(), 0), Eq(SEM_FAILED));
}

void ThreadSendFile2(void)
{

        ShmSendFile myShmSendObject1;
        myShmSendObject1.syncFileWithIPC("copyfileSendFileLast");
    
}

void ThreadReceiveFile2(void)
{
    ShmReceiveFile myShmReceiveObject1;
    myShmReceiveObject1.syncFileWithIPC("copyfileSendFileLast2");

}

TEST(ShmReceivefileAndShmSendfile, copyfileSendFileLast)
{
    CaptureStream stdcout(std::cout); //mute std::cout
    shm_unlink(ipcName.c_str());
    CreateRandomFile myRandomfile("copyfileSendFileLast",2,2);
    ASSERT_THAT(shm_open(ipcName.c_str(), O_RDWR,0), Eq(-1));
    ASSERT_THAT(sem_open(semSName.c_str(), 0), Eq(SEM_FAILED));

    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID2,ThreadReceiveFile2);
    usleep(10);
    start_pthread(&mThreadID1,ThreadSendFile2);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr);

    ASSERT_THAT(compareFiles("copyfileSendFileLast","copyfileSendFileLast2"), IsTrue);

    remove("copyfileSendFileLast");
    remove("copyfileSendFileLast2");
}


////////////////////// Killing a program: SendFile killed//////////////////////////


void ThreadShmSendFileKilledSend(void)
{
    IpcShmSendFileTest myShmSendObject1;
    myShmSendObject1.MokeSync("input.dat");
}

void ThreadShmSendFileKilledReceive(void)
{
    ShmReceiveFile myShmReceiveObject1{3};
    ASSERT_THROW(myShmReceiveObject1.syncFileWithIPC("output2.dat"), ipc_exception);
}

TEST(KillingAProgram, ShmSendFileKilled)
{
    std::string fileinput = "input.dat";
    std::string fileoutput = "output2.dat";
    CaptureStream stdcout(std::cout); //mute std::cout
    
    CreateRandomFile randomFile {fileinput,10, 10};

    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID1,ThreadShmSendFileKilledSend);
    start_pthread(&mThreadID2,ThreadShmSendFileKilledReceive);
    sleep(1);
    pthread_cancel(mThreadID1);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 

    remove(fileoutput.c_str());
}


////////////////////// Killing a program: ReceiveFile killed//////////////////////////


void ThreadShmReceiveFileKilledSend(void)
{
    ShmSendFile myShmSendObject1{3};
    ASSERT_THROW(myShmSendObject1.syncFileWithIPC("input.dat"), ipc_exception);
}

void ThreadShmReceiveFileKilledReceive(void)
{
    CaptureStream stdcout(std::cout); //mute std::cout
    IpcShmReceiveFileTest myShmReceiveObject1;
    myShmReceiveObject1.MokeSync();
}

TEST(KillingAProgram, ShmReceiveFileKilled)
{
    CaptureStream stdcout(std::cout); //mute std::cout
    std::string fileinput = "input.dat";
    std::string fileoutput = "output2.dat";
    
    CreateRandomFile randomFile {fileinput,1, 1};

    pthread_t mThreadID1, mThreadID2;
    start_pthread(&mThreadID2,ThreadShmReceiveFileKilledReceive);
    start_pthread(&mThreadID1,ThreadShmReceiveFileKilledSend);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 

    remove(fileoutput.c_str());
}



/////////////////////////// 2 ShmReceiveFile ///////////////////////////


void ThreadShmDoubleReceiverReceive(void)
{
    ShmReceiveFile myReceiver{2};
    ASSERT_THROW(myReceiver.syncFileWithIPC("output_pipe.dat"), ipc_exception);
}

void ThreadShmDoubleReceiverSend(void)
{
    ShmSendFile mySender{2};
    try 
    {
        mySender.syncFileWithIPC("input_pipe.dat");
    }
    catch (const ipc_exception &e)
    {
        std::cerr << "ThreadShmDoubleReceiverSend throw (expected)" << std::endl;
    }
}

TEST(IPCUsedByAnotherProgram,ShmDoubleReceiver)
{
    CaptureStream stdcout{std::cout};
    CreateRandomFile Randomfile("input_pipe.dat",10,10);
    pthread_t mThreadID1, mThreadID2, mThreadID3;
    start_pthread(&mThreadID1,ThreadShmDoubleReceiverReceive);
    start_pthread(&mThreadID3,ThreadShmDoubleReceiverReceive);
    start_pthread(&mThreadID2,ThreadShmDoubleReceiverSend);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ::pthread_join(mThreadID3, nullptr); 
    remove("output_pipe.dat");
}





///////////////////////////////// 2 ShmSendFile //////////////////////

void ThreadShmDoubleSenderReceive(void)
{
    ShmReceiveFile myReceiver{2};
    ASSERT_THROW(myReceiver.syncFileWithIPC("output_pipe.dat"), ipc_exception);
}

void ThreadShmDoubleSenderSend(void)
{
    ShmSendFile mySender{2};
    try
    {
        mySender.syncFileWithIPC("input_pipe.dat");
    }
    catch (const ipc_exception &e)
    {
        std::cerr << "ThreadShmDoubleSenderSend throw (expected)" << std::endl;
    }
}
void ThreadShmDoubleSenderSend2(void)
{
    ShmSendFile mySender{2};
    try
    {
        mySender.syncFileWithIPC("input_pipe2.dat");
    }
    catch (const ipc_exception &e)
    {
        std::cerr << "ThreadShmDoubleSenderSend2 throw (expected)" << std::endl;
    }
}

TEST(IPCUsedByAnotherProgram,ShmDoubleSender)
{
    CreateRandomFile Randomfile("input_pipe.dat",1,1);
    CreateRandomFile Randomfile2("input_pipe2.dat",1,1);
    pthread_t mThreadID1, mThreadID2, mThreadID3;
    start_pthread(&mThreadID1,ThreadShmDoubleSenderReceive);
    start_pthread(&mThreadID3,ThreadShmDoubleSenderSend);
    start_pthread(&mThreadID2,ThreadShmDoubleSenderSend2);
    ::pthread_join(mThreadID1, nullptr);
    ::pthread_join(mThreadID2, nullptr); 
    ::pthread_join(mThreadID3, nullptr); 
    remove("output_pipe.dat");
}