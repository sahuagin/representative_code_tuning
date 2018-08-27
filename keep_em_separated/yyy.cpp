// clang++ -std=c++14 -O3 -g yyy.cpp -lpthread -oyyy
// Here is program 1 (called yyy.cpp).
//
// It generates the data file used in the actual test program... but you can see that the performance for this simple program is HORRIBLE when run on the freebsd box (could be the filesystem itself - I know nothing about the freebsd box - it is basically a bunch of FILE writes).
//
// Note the amount of time spent in the OS for the freebsd run...
//clang++ -std=c++14 -O3 -g yyy.cpp -lpthread -oyyy
//
// Anyway, the code is attached...
//
// Built with this command...
//
// clang++ -std=c++14 -O3 -g yyy.cpp -lpthread -oyyy
//
//
//
//
// Here is the timing for running on udevtest012 - the AWS linux machine...
//
// $ mkdir -p /nvme/jhagins/Blarg/INPUT ; time ./yyy /nvme/jhagins/Blarg
//
// real 0m22.289s
// user 21m17.268s
// sys  0m42.004s
//
//
// Here is the timing for running it on the prod-freebsd box...
//
// $ mkdir -p /nvme/jhagins/Blarg/INPUT ; time ./yyy /nvme/jhagins/Blarg
//
// real 14m56.128s
// user 21m55.853s
// sys  526m13.904s
//
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <random>
#include <system_error>
#include <thread>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <cstdio>
// for sysctl to get ncpu
#include <sys/types.h>
#include <sys/sysctl.h>
// for cpuset
#include <sys/param.h>
#include <sys/cpuset.h>
// for pthread_np
#include <pthread_np.h>

#include "directories.h"


namespace {

  std::uint64_t
    hash(char const *str)
    {
      std::uint64_t result = 5381;
      int c;

      while ((c = *str++)) {
        result = ((result << 5) + result) + c;
      }
      return result;
    }

  /**
   * fp is length values only
   * hash_fp is for the hashes only XXX eventually this will be hash:len values
   * key_fp is for keyids only // XXX eventually, this won't be created here, it'll only be created by the xxx program
   * data_fp is for the variable length data only
   */
  bool open_files(int which, const std::string &dir, FILE *&hash_fp, FILE *&data_fp)
  {
    const std::string filenames[] = { IN_FILE_HASH, IN_FILE_DATA };
    uint16_t count(0);
    for(auto &filename : filenames)
    {
      char fbuf[256];
      ::memset(fbuf, 0, 256);
      //std::cerr << filename << std::endl;
      ::sprintf(fbuf, filename.c_str(), dir.c_str(), which);
      //std::cerr << fbuf << std::endl;
      FILE *tfp = ::fopen(fbuf, "w+");
      if (tfp == nullptr) {
        std::cerr << "error: \"" << fbuf << "\": " << strerror(errno) << std::endl;
        return false;
      }

      switch(count++)
      {
        case 0: hash_fp = tfp; 
                break;
        case 1: data_fp = tfp;
                break;
        default: std::cerr << "OH DOH!" << std::endl;
                 return false;
                 break;
      }
    }
    return true;
  }



  void make_file(std::string const &dir, int which, int core)
  {
    //char buf[1024];
    std::random_device rd;
    std::mt19937 rng(rd());
    auto char_dis = std::uniform_int_distribution<char>('A', 'Z');
    auto len_dis = std::uniform_int_distribution<std::uint16_t>(50, 140);


    // Create a cpu_set_t object representing a set of CPUs. Clear it and mark
    // only CPU i as set.
    cpuset_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);
    int rc = pthread_setaffinity_np(::pthread_self(),
        sizeof(cpuset_t), &cpuset);
    if (rc != 0) {
      std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }
    std::cerr << std::endl << "Affinity set to cpu " << core << "." << std::endl << std::endl;

    FILE *hash_fp=0, *data_fp=0;
    if(false == open_files(which, dir, hash_fp, data_fp))
    {
      return;
    }

    constexpr auto multiple{50000};
    std::vector<rec>hash_out(multiple);
    std::vector<char> data_out(140*multiple);

    auto accum_len{0};
    for (int i = 0; i < 305000000/100; ++i) {
      auto iterate = i%multiple;
      uint16_t len{len_dis(rng)};
      accum_len+=len;
      auto buf_start = data_out.size();
      for (std::uint16_t j = 0; j < len; ++j) {
        data_out.push_back(char_dis(rng));
      }
      //*next_iter++ = '\0';
      data_out.push_back('\0');
      //tmp_rec->hash = hash(&data_out[buf_start]);
      hash_out.push_back(rec{hash(&data_out[buf_start]), len});

      if(iterate==(multiple-1))
      {
        ::fwrite(&hash_out[0], sizeof(rec), hash_out.size(), hash_fp);
        ::fwrite(&data_out[0], sizeof(char) , accum_len, data_fp);
        accum_len=0;
        hash_out.empty();
        hash_out.reserve(multiple);
        data_out.empty();
        data_out.reserve(140*multiple);
      }
    }
    ::fclose(hash_fp);
    ::fclose(data_fp);

  }
}

int main(int argc, char * argv[])
{
  if (argc != 2) {
    std::cerr << "usage: yyy <path-to-data-files>" << std::endl;
    return 1;
  }
  std::string dir = argv[1];



  //int mib[4];
  //size_t len{sizeof(mib)};
  //int ret = ::sysctlnametomib("hw.ncpu", mib, &len);
  //int ncpu;
  //::sysctl(mib, 2, &ncpu, &len, NULL, 0);
  //std::cerr << "number of cpus: = " << ncpu << std::endl;
  auto ncpu = std::thread::hardware_concurrency();
  std::cerr << "hardware_concurrency: " << ncpu << std::endl;


  std::vector<std::thread> threads;
  size_t total_files{100}, total{0};
  while(total < total_files)
  {
    for (uint8_t cpus=0; (cpus < ncpu) && total<total_files; ++total) {
      threads.emplace_back(make_file, dir, total, cpus++);
    }

    for (auto &t: threads) t.join();
    threads.clear();
  }
  return 0;
}
