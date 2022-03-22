#ifndef GTEST_IPC_H
#define GTEST_IPC_H
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/lib/IpcCopyFile.h"
#include "../src/lib/IpcQueue.h"
#include <deque>
#include <vector>
#include <map>
#include <mqueue.h>
#include <fstream>
#include <pthread.h>

struct inputLineOpt{
  protocolList protocol;
  const char* filepath;
  std::vector<const char*> arguments;
};

bool compareFiles(const std::string& fileName1, const std::string& fileName2); // https://stackoverflow.com/questions/6163611/compare-two-files

std::vector<char> getRandomData(ssize_t size);


//Wrap function into thread to be able to test.
void *strip_void_star(void *arg);
void start_pthread(pthread_t *thread, void (*myFunction)(void));

//This class is just here to fake Command Line Opt
class FakeCmdLineOpt
{
  public:
    template<typename IT>
    FakeCmdLineOpt(IT begin, IT end) : arguments_(begin, end) //Overload number 5 of deque
    {
      argv_.clear();
      if (arguments_.empty() || arguments_[0] != "myProgramName")
        arguments_.emplace_front("myProgramName");
      for (auto &Opt : arguments_)
      {
        argv_.emplace_back(&Opt[0]); //pushing back the memory adress of the string argument
      }
      argc_ = argv_.size();
      argv_.push_back(NULL);
    };

    FakeCmdLineOpt(std::initializer_list<const char *> arguments_)
    : FakeCmdLineOpt(arguments_.begin(), arguments_.end()) 
    {
    };

    FakeCmdLineOpt &operator = (const FakeCmdLineOpt &); //Not copyable. On C++11
    
    FakeCmdLineOpt(const FakeCmdLineOpt &other):FakeCmdLineOpt(other.arguments_.begin(), other.arguments_.end())
    {
    };

    int argc() const
    {
      return argc_;
    };

    char * const* argv() const
    {
      return argv_.data();
    };

  private:
    std::deque<std::string> arguments_;
    std::vector<char *> argv_;
    int argc_;
};


class CaptureStream
{
  public:
  CaptureStream(std::ostream &to_swap) : to_swap_(to_swap)
  {
    old_buff_ = to_swap_.rdbuf(); // save the stream buffer
    to_swap_.rdbuf(buffer_.rdbuf()); // redirect the stream to buffer
    // 2 previous lines can be replaced by: old_buff_ = to_swap_.rdbuf(buffer_.rdbuf());
  }
  ~CaptureStream() {
    to_swap_.rdbuf(old_buff_); //redirect the stream to the original buffer
  }

  std::string str()
  {
    return buffer_.str();
  }

  protected:
   std::ostream &to_swap_;
   std::stringstream buffer_;
   std::streambuf *old_buff_;
};


class CreateRandomFile
{
  private:
    std::string file_name_;
    int nbOfBlocks_;
    int blockSize_;
  
  public:
    CreateRandomFile(const std::string &file_name, int nbOfBlocks, int blockSize): file_name_(file_name), nbOfBlocks_(nbOfBlocks), blockSize_(blockSize)
    {
      char buffer [100];
      snprintf(buffer, 100, "/bin/dd if=/dev/urandom of=%s bs=%dM count=%d status=none", file_name_.c_str(), blockSize_, nbOfBlocks_);
      std::system(const_cast<char*>(buffer)) ;

      ssize_t randomSize = rand() % 4096;
      std::vector<char> data = getRandomData(randomSize);

      std::fstream file;
      file.open(file_name.c_str(), std::ios::binary | std::fstream::in | std::fstream::out | std::fstream::app);
      file.write(&data[0], randomSize);

    }

    ~CreateRandomFile()
    {
      remove(file_name_.c_str());
    }

    const char* getFileName() const {return file_name_.c_str();}
};


class FileManipulationClassReader : public QueueSendFile
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

class FileManipulationClassWriter : public QueueReceiveFile
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
#endif /*GTEST_IPC_H */