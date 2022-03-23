#include "Gtest_Ipc.h"
#include <pthread.h>


bool compareFiles(const std::string& fileName1, const std::string& fileName2) // https://stackoverflow.com/questions/6163611/compare-two-files
{
  std::ifstream f1(fileName1, std::ifstream::binary|std::ifstream::ate);
  std::ifstream f2(fileName2, std::ifstream::binary|std::ifstream::ate);

  if (f1.fail() || f2.fail()) {
    std::cout << "file fail" <<std::endl;
    return false; //file problem
  }

  if (f1.tellg() != f2.tellg()) {

    std::cout << "size mismatch" <<std::endl;
    return false; //size mismatch
  }

  //seek back to beginning and use std::equal to compare contents
  f1.seekg(0, std::ifstream::beg);
  f2.seekg(0, std::ifstream::beg);
  return std::equal(std::istreambuf_iterator<char>(f1.rdbuf()),
                    std::istreambuf_iterator<char>(),
                    std::istreambuf_iterator<char>(f2.rdbuf()));

}

std::vector<char> getRandomData(ssize_t size)
{
  std::vector<char> retval;
  int randomDatafromUrandom = open("/dev/urandom", O_RDONLY);
  if (randomDatafromUrandom < 0)
  {
      throw std::runtime_error("error when oppening urandom.");
  }
  else
  {
      retval.resize(size);
      ssize_t result = read(randomDatafromUrandom, retval.data(), size);
      if (result < 0)
      {
        throw std::runtime_error("error when getting random data.");
      }
      if (result != size)
        std::cout << "Wrong file length."<<std::endl;
  }
  return retval;
}

void *strip_void_star(void *arg)
{
    void (*function)(void) = (void (*)())arg;
    function();
    return nullptr;
}

void start_pthread(pthread_t *thread, void (*myFunction)(void))
{
    ::pthread_create(thread, nullptr, strip_void_star, (void*) myFunction);
}