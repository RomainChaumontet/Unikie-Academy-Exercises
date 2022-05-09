#include "Gtest_Ipc.h"

HandyFunctions myToolBox;

bool compareFiles(const std::string& fileName1, const std::string& fileName2) // http://www.cplusplus.com/forum/general/94032/#msg504910
{
  std::ifstream f1(fileName1, std::ifstream::binary|std::ifstream::in);
  std::ifstream f2(fileName2, std::ifstream::binary|std::ifstream::in);

  if (f1.fail() && f2.fail()) {
    std::cout << "file fail" <<std::endl;
    return false; //file problem
  }

  if (myToolBox.returnFileSize(fileName1) != myToolBox.returnFileSize(fileName2)) {

    std::cout << "size mismatch" <<std::endl;
    std::cout << "First file size: " << myToolBox.returnFileSize(fileName1);
    std::cout << ". Second file size: " << myToolBox.returnFileSize(fileName2) <<std::endl;
    return false; //size mismatch
  }

  int bufferSize = 4096;
  char Buffer1[bufferSize];
  char Buffer2[bufferSize];

  do {
        f1.read(Buffer1, bufferSize);
        f2.read(Buffer2, bufferSize);
	      int numberOfRead = f1.gcount();
        if (numberOfRead != f2.gcount())
        {
          return false;
        }
        if (std::memcmp(Buffer1, Buffer2, numberOfRead) != 0)
        {
          return false;
        }
    } while (f1.good() || f2.good());

  return true;
}

std::vector<char> getRandomData()
{
  srand (time(NULL));
  ssize_t size = rand() % 4000;
  std::vector<char> retval(size);
  int randomDatafromUrandom = open("/dev/urandom", O_RDONLY);
  if (randomDatafromUrandom < 0)
  {
      throw std::runtime_error("error when oppening urandom.");
  }
  else
  {
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
