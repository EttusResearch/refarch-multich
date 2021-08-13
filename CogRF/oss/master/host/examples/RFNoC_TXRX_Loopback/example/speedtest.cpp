#include <iostream>
#include <fstream>
#include <chrono>
#include <memory>
#include <stdio.h>
#include <cstring>
#include <boost/thread/thread.hpp>
#ifdef __linux__
#include <unistd.h>

#endif
using namespace std;
using namespace std::chrono;
const size_t sz = 512 * 1024 * 1024;
const int numiter = 20;
const size_t bufsize = 1024 * 1024;


void test(int thread, std::string test){
  unique_ptr<char[]> data(new char[sz]);
  unique_ptr<char[]> buf(new char[bufsize]);
  for (size_t p = 0; p < sz; p += 16) {
     memcpy(&data[p], "BINARY.DATA.....", 16);
  }
  std::string file = "file" + to_string(thread) + ".binary";
  unlink(file.c_str());
  int64_t total = 0;
  if (test == "fstream") {
    cout << "fstream mode\n";
    ofstream myfile(file, ios::out | ios::binary);
    if (!myfile) {
      cerr << "open failed\n"; exit;
    }
    myfile.rdbuf()->pubsetbuf(buf.get(), bufsize); // IMPORTANT
    for (int i = 0; i < numiter; ++i) {
      auto tm1 = high_resolution_clock::now();
      myfile.write(data.get(), sz);
      if (!myfile)
        cerr << "write failed\n";
      auto tm = (duration_cast<milliseconds>(high_resolution_clock::now() - tm1).count());
      cout << tm << " ms\n";
      total += tm;
    }
    myfile.close();
  }
  else {
    cout << "fopen mode\n";
    FILE* pFile = fopen("file.binary", "wb");
    if (!pFile) {
      cerr << "open failed\n"; exit;
    }
    setvbuf(pFile, buf.get(), _IOFBF, bufsize); // NOT important
    auto tm1 = high_resolution_clock::now();
    for (int i = 0; i < numiter; ++i) {
      auto tm1 = high_resolution_clock::now();
      if (fwrite(data.get(), sz, 1, pFile) != 1)
        cerr << "write failed\n";
      auto tm = (duration_cast<milliseconds>(high_resolution_clock::now() - tm1).count());
      cout << tm << " ms\n";
      total += tm;
    }
    fclose(pFile);
    auto tm2 = high_resolution_clock::now();
  }
  cout << "Thread: " << thread << " Total: " << total << " ms, " << (sz*numiter * 1000 / (1024.0 * 1024 * total)) << " MB/s\n";

}
int main(int argc, char**argv)
{
    boost::thread_group thread_group;

    for (int i = 0; i < 32 ;i++){
        std::cout << "Spawning Thread: " << i << std::endl;
        thread_group.create_thread(std::bind(&test, i, "fstream"));
        
    }
    thread_group.join_all();
    std::cout << "Done..." << std::endl;
 
}