#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/lib/IpcCopyFile.h"
#include "Gtest_Ipc.h"
#include <deque>
#include <vector>
#include <sstream>


using ::testing::Eq;
using ::testing::StrEq;
using ::testing::IsTrue;
using ::testing::IsFalse;



class FileManipulationClassReader : public Reader
{
    public:
        void modifyBufferToWrite(const std::string &data)
        {
            bufferSize_ = data.size();
            buffer_ = std::vector<char> (data.begin(), data.end());
            return;
        }
        void modifyBufferToWrite(const std::vector<char> &data)
        {
            bufferSize_ = data.size();
            buffer_ = data;
            return;
        }

        std::string bufferForReading() const
        {
            return std::string (buffer_.begin(), buffer_.end());
        }

        std::vector<char> getBufferRead() const
        {
            return buffer_;
        }
};

class FileManipulationClassWriter : public Writer
{
  public:
        void modifyBufferToWrite(const std::string &data)
        {
            bufferSize_ = data.size();
            buffer_ = std::vector<char> (data.begin(), data.end());
            return;
        }
        void modifyBufferToWrite(const std::vector<char> &data)
        {
            bufferSize_ = data.size();
            buffer_ = data;
            return;
        }

        std::string bufferForReading() const
        {
            return std::string (buffer_.begin(), buffer_.end());
        }

        std::vector<char> getBufferRead() const
        {
            return buffer_;
        }
};

TEST(Constructor, ProtocolName)
{
    Reader dummyCopyFileReader;
    Writer dummyCopyFileWriter;
    EXPECT_THAT(dummyCopyFileReader.getName(), StrEq("ipcCopyFile"));
    EXPECT_THAT(dummyCopyFileWriter.getName(), StrEq("ipcCopyFile"));
}

TEST(TestSimpleMethods, ChangeAndGetName)
{
    Reader dummyCopyFileReader;
    EXPECT_THAT(dummyCopyFileReader.changeName("Coucou"), StrEq("Coucou"));
    EXPECT_THAT(dummyCopyFileReader.getName(), StrEq("Coucou"));
    {
        CaptureStream stdcerr{std::cerr};
        EXPECT_THAT(dummyCopyFileReader.changeName(""), StrEq("Coucou")); //will not change the name
        EXPECT_THAT(stdcerr.str(), StrEq("Error. Trying to change the name of protocol exchange to null. Keep it unchanged.\n"));
    }
    EXPECT_THAT(dummyCopyFileReader.getName(), StrEq("Coucou"));


    Writer dummyCopyFileWriter;
    EXPECT_THAT(dummyCopyFileWriter.changeName("Coucou"), StrEq("Coucou"));
    EXPECT_THAT(dummyCopyFileWriter.getName(), StrEq("Coucou"));
    {
        CaptureStream stdcerr{std::cerr};
        EXPECT_THAT(dummyCopyFileWriter.changeName(""), StrEq("Coucou")); //will not change the name
        EXPECT_THAT(stdcerr.str(), StrEq("Error. Trying to change the name of protocol exchange to null. Keep it unchanged.\n"));
    } 
    EXPECT_THAT(dummyCopyFileWriter.getName(), StrEq("Coucou"));
}

TEST(TestSimpleMethods, GetBufferSize)
{
    Reader dummyCopyFileReader;
    EXPECT_THAT(dummyCopyFileReader.getBufferSize(),Eq(4096));

    Writer dummyCopyFileWriter;
    EXPECT_THAT(dummyCopyFileWriter.getBufferSize(),Eq(4096));
}

TEST(FileManipulation, OpenFile)
{
    
    Reader dummyCopyFileReader;
    Writer dummyCopyFileWriter;
    {
        std::string dummyFile = "IamTestingToOpenThisFile";
        EXPECT_THROW(dummyCopyFileReader.openFile(dummyFile.c_str()),std::runtime_error);
    }

    std::string fileToOpenForWriting = "fileToOpenForWriting";
    EXPECT_NO_THROW(dummyCopyFileWriter.openFile(fileToOpenForWriting.c_str()));
    EXPECT_NO_THROW(dummyCopyFileReader.openFile(fileToOpenForWriting.c_str()));

    
    {
        Writer openAnotherTime;
        CaptureStream stdcout{std::cout};
        EXPECT_NO_THROW(openAnotherTime.openFile(fileToOpenForWriting.c_str()));
        EXPECT_THAT(stdcout.str(), StrEq("The file specified to write in already exists. Data will be erased before proceeding.\n"));
    }

    remove(fileToOpenForWriting.c_str());

    {
        
        CreateRandomFile randomFile {"testfile.dat", 10, 1};
        Reader openRandomFile;
        EXPECT_NO_THROW(openRandomFile.openFile(randomFile.getFileName()));
        {
            Writer openRandomFileWriter;
            CaptureStream stdcout{std::cout};
            EXPECT_NO_THROW(openRandomFileWriter.openFile(randomFile.getFileName()));
            EXPECT_THAT(stdcout.str(), StrEq("The file specified to write in already exists. Data will be erased before proceeding.\n"));
        }
    }
}


TEST(FileManipulation, ReadAndWriteSimpleFiles)
{
    FileManipulationClassWriter writingToAFile;
    FileManipulationClassReader readingAFile;
    std::string data = "I expect these data will be writen in the file.\n";
    writingToAFile.modifyBufferToWrite(data);

    //Test writing while the file is not opened
    EXPECT_THROW(writingToAFile.syncFileWithBuffer(), std::runtime_error);

    //Test reading while the file is not opened
    EXPECT_THROW(readingAFile.syncFileWithBuffer(), std::runtime_error);

    ASSERT_NO_THROW(writingToAFile.openFile("TmpFile.txt")); //file empty
    EXPECT_NO_THROW(writingToAFile.syncFileWithBuffer());
    writingToAFile.closeFile();
    
    EXPECT_NO_THROW(readingAFile.openFile("TmpFile.txt"));
    EXPECT_NO_THROW(readingAFile.syncFileWithBuffer());
    EXPECT_THAT(readingAFile.bufferForReading(), StrEq(data));
    readingAFile.closeFile();
    
    writingToAFile.modifyBufferToWrite("");
    {
        CaptureStream stdcout{std::cout}; //don't want to see std::cout in the test log
        ASSERT_NO_THROW(writingToAFile.openFile("TmpFile.txt")); //file empty
    }
    EXPECT_NO_THROW(writingToAFile.syncFileWithBuffer());
    writingToAFile.closeFile();

    EXPECT_NO_THROW(readingAFile.openFile("TmpFile.txt"));
    EXPECT_NO_THROW(readingAFile.syncFileWithBuffer());
    EXPECT_THAT(readingAFile.bufferForReading(), StrEq(""));
    readingAFile.closeFile();

    remove("TmpFile.txt");
}


TEST(FileManipulation, ReadAndWriteComplexFiles)
{
    std::string nameOfRandomFile = "testfile.dat";
    CreateRandomFile randomFile (nameOfRandomFile, 10, 1);
    size_t sizeOfOriginalFile = returnFileSize(nameOfRandomFile);
    std::string fileForWriting = "testfilecpy.dat";
    FileManipulationClassWriter writingToAFile;
    FileManipulationClassReader readingAFile;

    size_t dataRead = 0;
    ASSERT_NO_THROW(readingAFile.openFile(nameOfRandomFile.c_str()));
    ASSERT_NO_THROW(writingToAFile.openFile(fileForWriting.c_str()));


    while (dataRead < sizeOfOriginalFile)
    {
        ASSERT_NO_THROW(readingAFile.syncFileWithBuffer());
        dataRead += readingAFile.getBufferSize();
        writingToAFile.modifyBufferToWrite(readingAFile.getBufferRead());//copy buffer read to buffer write
        ASSERT_NO_THROW(writingToAFile.syncFileWithBuffer());
    }
    writingToAFile.closeFile();
    readingAFile.closeFile();

    ASSERT_THAT(sizeOfOriginalFile, Eq(returnFileSize(fileForWriting.c_str())));

    remove(nameOfRandomFile.c_str());
    remove(fileForWriting.c_str());

}
