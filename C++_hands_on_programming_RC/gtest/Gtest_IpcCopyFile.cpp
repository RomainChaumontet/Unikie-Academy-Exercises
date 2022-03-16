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

TEST(TestSimpleMethods, ChangeAndGetBufferSize)
{
    Reader dummyCopyFileReader;
    EXPECT_THAT(dummyCopyFileReader.getBufferSize(),Eq(4096));
    EXPECT_THAT(dummyCopyFileReader.changeBufferSize(9999),Eq(9999));
    EXPECT_THAT(dummyCopyFileReader.getBufferSize(),Eq(9999));
    {
        CaptureStream stdcerr{std::cerr};
        EXPECT_THAT(dummyCopyFileReader.changeBufferSize(0), Eq(9999)); //will not change the name
        EXPECT_THAT(stdcerr.str(), StrEq("Error. Trying to change the size of the buffer to 0. Keep it unchanged.\n"));
    }
    EXPECT_THAT(dummyCopyFileReader.getBufferSize(),Eq(9999));

    Writer dummyCopyFileWriter;
    EXPECT_THAT(dummyCopyFileWriter.getBufferSize(),Eq(4096));
    EXPECT_THAT(dummyCopyFileWriter.changeBufferSize(9999),Eq(9999));
    EXPECT_THAT(dummyCopyFileWriter.getBufferSize(),Eq(9999));
    {
        CaptureStream stdcerr{std::cerr};
        EXPECT_THAT(dummyCopyFileWriter.changeBufferSize(0), Eq(9999)); //will not change the name
        EXPECT_THAT(stdcerr.str(), StrEq("Error. Trying to change the size of the buffer to 0. Keep it unchanged.\n"));
    }
    EXPECT_THAT(dummyCopyFileWriter.getBufferSize(),Eq(9999));
}


TEST(FileManipulation, OpenFile)
{
    
    Reader dummyCopyFileReader;
    Writer dummyCopyFileWriter;
    {
        std::string dummyFile = "IamTestingToOpenThisFile";
        CaptureStream stdcerr{std::cerr};
        EXPECT_THAT(dummyCopyFileReader.openFile(dummyFile.c_str()),IsFalse());
        EXPECT_THAT(stdcerr.str(), StrEq("Error. Trying to open a file for reading which does not exist.\n"));
    }

    std::string fileToOpenForWriting = "fileToOpenForWriting";
    EXPECT_THAT(dummyCopyFileWriter.openFile(fileToOpenForWriting.c_str()), IsTrue());
    EXPECT_THAT(dummyCopyFileReader.openFile(fileToOpenForWriting.c_str()),IsTrue());

    
    {
        Writer openAnotherTime;
        CaptureStream stdcout{std::cout};
        EXPECT_THAT(openAnotherTime.openFile(fileToOpenForWriting.c_str()),IsTrue());
        EXPECT_THAT(stdcout.str(), StrEq("The file specified to write in already exists. Data will be erased before proceeding.\n"));
    }

    remove(fileToOpenForWriting.c_str());

    {
        
        CreateRandomFile randomFile {"testfile.dat", 10, 1};
        Reader openRandomFile;
        EXPECT_THAT(openRandomFile.openFile(randomFile.getFileName()),IsTrue());
        {
            Writer openRandomFileWriter;
            CaptureStream stdcout{std::cout};
            EXPECT_THAT(openRandomFileWriter.openFile(randomFile.getFileName()),IsTrue());
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

    { //Test writing while the file is not opened
        CaptureStream stdcerr{std::cerr};
        EXPECT_THAT(writingToAFile.syncFileWithBuffer(), IsFalse());
        EXPECT_THAT(stdcerr.str(), StrEq("Error, trying to write to a file which is not opened.\n"));
    }
    { //Test reading while the file is not opened
        CaptureStream stdcerr{std::cerr};
        EXPECT_THAT(readingAFile.syncFileWithBuffer(), IsFalse());
        EXPECT_THAT(stdcerr.str(), StrEq("Error, trying to read a file which is not opened.\n"));
    }

    ASSERT_THAT(writingToAFile.openFile("TmpFile.txt"), IsTrue()); //file empty
    EXPECT_THAT(writingToAFile.syncFileWithBuffer(),IsTrue());
    writingToAFile.closeFile();
    
    EXPECT_THAT(readingAFile.openFile("TmpFile.txt"), IsTrue());
    EXPECT_THAT(readingAFile.syncFileWithBuffer(), IsTrue());
    EXPECT_THAT(readingAFile.bufferForReading(), StrEq(data));
    readingAFile.closeFile();
    
    writingToAFile.modifyBufferToWrite("");
    {
        CaptureStream stdcout{std::cout};
        ASSERT_THAT(writingToAFile.openFile("TmpFile.txt"), IsTrue()); //file empty
    }
    EXPECT_THAT(writingToAFile.syncFileWithBuffer(),IsTrue());
    writingToAFile.closeFile();

    EXPECT_THAT(readingAFile.openFile("TmpFile.txt"), IsTrue());
    EXPECT_THAT(readingAFile.syncFileWithBuffer(), IsTrue());
    EXPECT_THAT(readingAFile.bufferForReading(), StrEq(""));
    readingAFile.closeFile();

    remove("TmpFile.txt");
}

TEST(FileManipulation, ReadAndWriteComplexFiles)
{
    std::string nameOfRandomFile = "testfile.dat";
    CreateRandomFile randomFile {nameOfRandomFile.c_str(), 10, 1};
    size_t sizeOfOriginalFile = returnFileSize(nameOfRandomFile);
    std::string fileForWriting = "testfilecpy.dat";
    FileManipulationClassWriter writingToAFile;
    FileManipulationClassReader readingAFile;

    size_t dataRead = 0;
    ASSERT_THAT(readingAFile.openFile(nameOfRandomFile.c_str()), IsTrue());
    ASSERT_THAT(writingToAFile.openFile(fileForWriting.c_str()), IsTrue());


    while (dataRead < sizeOfOriginalFile)
    {
        ASSERT_THAT(readingAFile.syncFileWithBuffer(),IsTrue());
        //std::cout << "buffersize: " << readingAFile.getBufferSize() << std::endl;
        dataRead += readingAFile.getBufferSize();
        //std::cout << "size of file:" << sizeOfOriginalFile << ". Data already read:" << dataRead << std::endl;
        writingToAFile.changeBufferSize(readingAFile.getBufferSize());
        writingToAFile.modifyBufferToWrite(readingAFile.getBufferRead());//copy buffer read to buffer write
        ASSERT_THAT(writingToAFile.syncFileWithBuffer(),IsTrue());
    }
    writingToAFile.closeFile();
    readingAFile.closeFile();

    ASSERT_THAT(sizeOfOriginalFile, Eq(returnFileSize(fileForWriting.c_str())));

    remove(nameOfRandomFile.c_str());
    remove(fileForWriting.c_str());

}
