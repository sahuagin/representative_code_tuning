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

  void make_file(std::string const &dir, int which, int core)
  {
    char buf[1024];
    std::random_device rd;
    std::mt19937 rng(rd());
    auto char_dis = std::uniform_int_distribution<char>('A', 'Z');
    auto len_dis = std::uniform_int_distribution<std::uint16_t>(50, 140);

    std::uint64_t key = 0u;

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


    char fbuf[256];
    ::sprintf(fbuf, "%s/INPUT/file.%03d", dir.c_str(), which);
    FILE *fp = ::fopen(fbuf, "w+");
    if (fp == nullptr) {
      std::cerr << "error: \"" << fbuf << "\": " << strerror(errno) << std::endl;
      return;
    }

    for (int i = 0; i < 305000000/100; ++i) {
      auto const len = len_dis(rng);
      for (std::uint16_t j = 0; j < len; ++j) {
        buf[j] = char_dis(rng);
      }
      buf[len] = '\0';
      std::uint64_t h = hash(buf);
      ::fwrite(&key, sizeof(key), 1, fp);
      ::fwrite(&h, sizeof(h), 1, fp);
      ::fwrite(&len, sizeof(len), 1, fp);
      ::fwrite(buf, 1, len, fp);
    }
    ::fclose(fp);
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
