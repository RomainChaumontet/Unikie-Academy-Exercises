#include "Gtest_Ipc.h"
#include <time.h>
#include <stdio.h> 
#include <stdlib.h>

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



TEST(HandyFunctions, ToolBoxMemberAccess)
{
    HandyFunctions ToolBox;
    EXPECT_THAT(ToolBox.getDefaultBufferSize(),Eq(4096));
    EXPECT_THAT(ToolBox.getKey(),Eq(151563468));
    EXPECT_THAT(ToolBox.getMaxAttempt(),Eq(30));
}

TEST(HandyFunctions, checkFilePath)
{
    HandyFunctions ToolBox;
    std::string normalFilePath = "myfilePath/";
    std::string exceedFilePath(PATH_MAX+1, 'c');
    exceedFilePath.append("/");
    std::string normalFileName = "myFileName";
    std::string exceedFileName(NAME_MAX+1, 'c');
    EXPECT_NO_THROW(ToolBox.checkFilePath(normalFilePath+normalFileName));
    EXPECT_NO_THROW(ToolBox.checkFilePath(normalFileName));
    EXPECT_THROW(ToolBox.checkFilePath(exceedFilePath+normalFileName), arguments_exception);
    EXPECT_THROW(ToolBox.checkFilePath(exceedFilePath+exceedFileName), arguments_exception);
    EXPECT_THROW(ToolBox.checkFilePath("/" + exceedFilePath+normalFileName), arguments_exception);
    EXPECT_THROW(ToolBox.checkFilePath("/" + exceedFilePath+exceedFileName), arguments_exception);
    EXPECT_THROW(ToolBox.checkFilePath(normalFilePath+exceedFileName), arguments_exception);
    EXPECT_THROW(ToolBox.checkFilePath("/"+normalFilePath+exceedFileName), arguments_exception);
}

TEST(HandyFunctions, chefIfFileExists)
{
    HandyFunctions ToolBox; 
    std::string fileName = "myFileName.dat";
    EXPECT_THAT(ToolBox.checkIfFileExists(fileName), IsFalse()) << ToolBox.checkIfFileExists(fileName) << std::endl;
    CreateRandomFile myRandomFile(fileName, 1,1);
    std::fstream file;
    file.open(fileName, std::ios::in | std::ios::binary);
    EXPECT_THAT(file.is_open(), IsTrue());
    EXPECT_THAT(ToolBox.checkIfFileExists(fileName), IsTrue()) << ToolBox.checkIfFileExists(fileName) << std::endl;
}

TEST(HandyFunctions, returnFileSize)
{
    srand (time(NULL));
    int a = rand() % 10+1;
    int b = rand() % 10+1;
    HandyFunctions ToolBox; 
    std::string fileName = "myFileName.dat";
    CreateRandomFile myRandomFile(fileName, a,b, false);
    EXPECT_THAT(ToolBox.returnFileSize(fileName), Eq(1024*1024*a*b));
}

TEST(HandyFunctions, enoughSpaceAvailable)
{
    HandyFunctions ToolBox; 
    size_t maxSize = std::numeric_limits<size_t>::max();
    EXPECT_THAT(ToolBox.enoughSpaceAvailable(maxSize), IsFalse());
    EXPECT_THAT(ToolBox.enoughSpaceAvailable(10), IsTrue());
}

TEST(HandyFunctions, printInstructions)
{
    HandyFunctions ToolBox; 
    CaptureStream stdcout(std::cout);
    ToolBox.printInstructions();
    EXPECT_THAT(stdcout.str(), EndsWith("ipc channel name should not be more than NAME_MAX character (usually 255).\n"));
}

TEST(HandyFunctions, updatePrintingElements)
{
    HandyFunctions ToolBox; 
    {
        CaptureStream stdcout(std::cout);
        ToolBox.updatePrintingElements("Testing.");
        EXPECT_THAT(stdcout.str(), ContainsRegex("Testing."));
    }
    {
        CaptureStream stdcout(std::cout);
        ToolBox.updatePrintingElements("Testing.");
        EXPECT_THAT(stdcout.str(), StrEq("")); //no refreshing because < 50ms
    }
}

TEST(HandyFunctions, nap)
{
    HandyFunctions ToolBox; 
    std::chrono::time_point<std::chrono::steady_clock> start =std::chrono::steady_clock::now();
    EXPECT_NO_THROW(ToolBox.nap(10));
    std::chrono::time_point<std::chrono::steady_clock> now =std::chrono::steady_clock::now();
    EXPECT_THAT(std::chrono::duration_cast<std::chrono::milliseconds>(now-start).count(),Ge(10));//sleep more than 10ms
    EXPECT_THAT(std::chrono::duration_cast<std::chrono::milliseconds>(now-start).count(),Le(20));//sleep less than 20ms
}

TEST(HandyFunctions, getTime)
{
    struct timespec ts;
    struct timespec ts2;
    HandyFunctions ToolBox; 
    
    EXPECT_NO_THROW(ToolBox.getTime(ts));
    EXPECT_NO_THROW(ToolBox.getTime(ts2));
    EXPECT_THAT(ts2.tv_nsec-ts.tv_nsec, Le(10000000)); // less than 10ms (actually it's around 120ns but in case of RoundRobin...)
}

TEST(HandyFunctions, printFileSize)
{
    HandyFunctions ToolBox;
    {
        CaptureStream stdcout(std::cout);
        EXPECT_NO_THROW(ToolBox.printFileSize(84597970829312));
        EXPECT_THAT(stdcout.str(), StrEq("Transferring a file which size: 78788GB 0MB 0KB 0B.\n"));
    }
    {
        CaptureStream stdcout(std::cout);
        EXPECT_NO_THROW(ToolBox.printFileSize(2097152));
        EXPECT_THAT(stdcout.str(), StrEq("Transferring a file which size: 0GB 2MB 0KB 0B.\n"));
    }
    {
        CaptureStream stdcout(std::cout);
        EXPECT_NO_THROW(ToolBox.printFileSize(523));
        EXPECT_THAT(stdcout.str(), StrEq("Transferring a file which size: 0GB 0MB 0KB 523B.\n"));
    }
    {
        CaptureStream stdcout(std::cout);
        EXPECT_NO_THROW(ToolBox.printFileSize(293888));
        EXPECT_THAT(stdcout.str(), StrEq("Transferring a file which size: 0GB 0MB 287KB 0B.\n"));
    }
}

TEST(HandyFunctions, compareFileNames)
{
    HandyFunctions ToolBox;

    EXPECT_THROW(ToolBox.compareFileNames("test", "test"), arguments_exception);
    EXPECT_NO_THROW(ToolBox.compareFileNames("test1", "test2"));

    char currentDir[PATH_MAX];
    getcwd(currentDir, PATH_MAX);
    EXPECT_THROW(ToolBox.compareFileNames(std::string(currentDir) + '/' + "test", "test"), arguments_exception);
}

TEST(FileHandler, constructors)
{
    HandyFunctions toolBox;
    std::string fileName = "myFile";

    EXPECT_THROW(Reader(fileName, &toolBox), file_exception); //file does not exists
    EXPECT_NO_THROW(Writer(fileName, &toolBox));
    EXPECT_NO_THROW(Reader(fileName, &toolBox));

    {
        CaptureStream stdcout(std::cout);
        EXPECT_NO_THROW(Writer(fileName, &toolBox));
        EXPECT_THAT(stdcout.str(), StrEq("The file specified to write in already exists. Data will be erased before proceeding.\n"));
    }

    remove(fileName.c_str());

    //creating a file only write permission
    int fd = open(fileName.c_str(), O_WRONLY|O_CREAT|O_TRUNC, S_IWGRP); //create with only write permission
    EXPECT_THROW(Reader(fileName, &toolBox), file_exception);
    close(fd);
    remove(fileName.c_str());

    //creating a file with only read permission
    fd = open(fileName.c_str(), O_RDONLY|O_CREAT|O_TRUNC, S_IRUSR);
    CaptureStream stdcout(std::cout);
    EXPECT_THROW(Writer(fileName, &toolBox), file_exception); 
    close(fd);
    EXPECT_THAT(remove(fileName.c_str()),Eq(0));

}

TEST(FileHandler, ReadAndWrite)
{
    HandyFunctions toolBox;
    std::string fileName = "myFile";
    std::vector<char> buffer(toolBox.getDefaultBufferSize(),'c');
    std::vector<char> buffer2(toolBox.getDefaultBufferSize(),'d');

    {
        FileHandler myFileHandler(fileName, &toolBox);
        //Assert throw because the file is not open.
        ASSERT_THROW(myFileHandler.readFile(buffer.data(), toolBox.getDefaultBufferSize()), file_exception);
        ASSERT_THROW(myFileHandler.writeFile(buffer.data(), toolBox.getDefaultBufferSize()), file_exception);
    }

    {
        Writer myWriter(fileName, &toolBox); //create the file and open it
        Reader myReader(fileName, &toolBox); //open the file

        myWriter.writeFile(buffer.data(), toolBox.getDefaultBufferSize());
        std::vector<char> bufferForReading(toolBox.getDefaultBufferSize());
        myReader.readFile(bufferForReading.data(), toolBox.getDefaultBufferSize());
        EXPECT_THAT(std::equal(bufferForReading.begin(), bufferForReading.end(), buffer.begin()), IsTrue());

        EXPECT_THAT(myReader.readFile(bufferForReading.data(), toolBox.getDefaultBufferSize()),Eq(0));
        remove(fileName.c_str());
    }
    {   //testing offsets
        Writer myWriter(fileName, &toolBox); //create the file and open it
        Reader myReader(fileName, &toolBox); //open the file
        myWriter.writeFile(buffer.data(), toolBox.getDefaultBufferSize());
        myWriter.writeFile(buffer2.data(), toolBox.getDefaultBufferSize());
        
        std::vector<char> bufferForReading(toolBox.getDefaultBufferSize());
        myReader.readFile(bufferForReading.data(), toolBox.getDefaultBufferSize());
        EXPECT_THAT(std::equal(bufferForReading.begin(), bufferForReading.end(), buffer.begin()), IsTrue());
        myReader.readFile(bufferForReading.data(), toolBox.getDefaultBufferSize());
        EXPECT_THAT(std::equal(bufferForReading.begin(), bufferForReading.end(), buffer2.begin()), IsTrue());
        remove(fileName.c_str());
    }

    MockToolBox toolBoxMocked;
    { //test ennoughSpaceAvailable and checkIfFileExists in the Reader constructor
        EXPECT_NO_THROW(Writer myWriter(fileName, &toolBox)); //create the file and open it

        EXPECT_CALL(toolBoxMocked, checkIfFileExists(A<const std::string&>()))
            .WillOnce(Return(false))
            .WillOnce(Return(true));
        EXPECT_CALL(toolBoxMocked, returnFileSize(A<const std::string&>()))
            .WillRepeatedly(Return(1));   
        EXPECT_CALL(toolBoxMocked,enoughSpaceAvailable(A<size_t>()))
            .WillOnce(Return(false));
        
        EXPECT_THROW(Reader(fileName, &toolBoxMocked), file_exception);
        EXPECT_THROW(Reader(fileName, &toolBoxMocked), system_exception);
        remove(fileName.c_str());
    }
}

class HeaderTester : public Header
{
    public:
        HeaderTester(size_t key, size_t fileSize, HandyFunctions* toolbox):Header(key,fileSize,toolbox){};
        HeaderTester(size_t key, HandyFunctions* toolbox):Header(key,toolbox){};
        
        std::vector<size_t>& getVector()
        {
            return headerVector;
        }
};

TEST(Header, ConstructorAndMethod)
{
    HandyFunctions toolBox;
    size_t key = rand() %1000000;
    size_t fileSize = rand() %1000000;

    {
        HeaderTester myHeaderForTesting(key, fileSize, &toolBox);
        EXPECT_THAT(myHeaderForTesting.getVector().size(), Eq(toolBox.getDefaultBufferSize()));
        EXPECT_THAT(myHeaderForTesting.getKey(), Eq(key));
        EXPECT_THAT(myHeaderForTesting.getFileSize(), Eq(fileSize));
    }
    
    {        
        HeaderTester myHeaderForTesting(key, &toolBox);
        EXPECT_THAT(myHeaderForTesting.getVector().size(), Eq(toolBox.getDefaultBufferSize()));
        EXPECT_THAT(myHeaderForTesting.getKey(), Eq(key));
    }
    
}