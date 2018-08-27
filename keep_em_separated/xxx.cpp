// clang++ -std=c++14 -O3 -g xxx.cpp -lpthread -oxxx
//
// This is the "test" program that is somewhat similar to what the actual code does...
//
//
// Built with this command...
//
// clang++ -std=c++14 -O3 -g xxx.cpp -lpthread -oxxx
//
//
// Before running, must have run the "yyy" program first because it generates the input for the test program.
//
//
// Here is the timing for running on udevtest012 - the AWS linux machine...
//
// $ time ./xxx /nvme/jhagins/Blarg
//
// real 6m43.322s
// user 21m42.512s
// sys  100m41.928s
//
// $ time ./xxx /nvme/jhagins/Blarg
//
// real 39m26.312s
// user 20m19.977s
// sys  682m24.402s
//
//
//
// Here is the source code - I can make it simpler if you want...
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
  struct FD
  {
    explicit FD(int f) : fd(f) { }
    FD(char const *fname, int flags)
      : FD(::open(fname, flags))
    {
      if (fd < 0) {
        throw std::system_error(errno, std::system_category());
      }
    }
    FD(FD const &) = delete;
    FD & operator=(FD const &) = delete;
    FD(FD && that) : fd(that.fd) { that.fd = -1; }
    FD & operator=(FD && that) { fd = that.fd; that.fd = -1; return *this; }
    ~FD() { if (fd >= 0) ::close(fd); }
    operator int () const { return fd; };
    int fd;
  };

  std::pair<void *, std::size_t>
    map_file(int fd, bool write=true, bool randfile=false)
    {
      std::pair<void *, std::size_t> result{nullptr, 0u};
      struct stat status;
      if (::fstat(fd, &status) != 0) {
        return result;
      }
      auto filesz = std::size_t(status.st_size);
      int map_flags=0;
      int read_write=PROT_READ;
      if(true==write)
      {
        read_write |= PROT_WRITE;
        map_flags  |= MAP_SHARED;
      }
      else
      {
        map_flags |= MAP_PRIVATE;
      }
#if defined(__linux__)
      // nothing
#elif defined(__FreeBSD__)
      map_flags = map_flags | MAP_NOCORE | MAP_ALIGNED_SUPER | MAP_NOSYNC;
#else
      map_flags = map_flags | MAP_NOCORE;
#endif
      auto addr = ::mmap(nullptr, filesz, read_write, map_flags, fd, 0);
      if (addr == MAP_FAILED) {
        return result;
      }
#if defined(__linux__)
      if (::madvise(addr, filesz, MADV_DONTDUMP) != 0) {
        return result;
      }
#elif defined(__FreeBSD__)
      if(true==randfile)
      {
        if (::madvise(addr, filesz, MADV_RANDOM) != 0) {
          return result;
        }
      }
      else
      {
        if (::madvise(addr, filesz, MADV_SEQUENTIAL) != 0) {
          return result;
        }
      }
#endif
      result.first = addr;
      result.second = filesz;
      return result;
    }

  //constexpr auto num_tables = std::size_t(64);

  struct Table
  {
    static constexpr auto table_size = std::size_t(33554432);
    static constexpr auto table_mask = table_size - 1;

    std::pair<void *, std::size_t> tabmap, datmap;
    struct Bucket
    {
      std::uint64_t offset;
      std::uint64_t hash;
    };
    Bucket *bucket;
    char *data;
    std::uint64_t datoffs = 42u;

    explicit Table(std::string const &dirname)
    {
      char buf[2*1024];

      ::mkdir(dirname.c_str(), 0775);
      // make HT
      ::mkdir( (dirname + HT).c_str(), 0775);
      // make HT/table
      ::mkdir( (dirname + HT_TABLE).c_str(), 0775);
      // make HT/data
      ::mkdir( (dirname + HT_DATA).c_str(), 0775);

      //::sprintf(buf, "%s/tab.XXXXXX", dirname.c_str());
      ::sprintf(buf, TAB_TEMP.c_str(), (dirname + HT_TABLE).c_str());
      auto tab_fd = FD(::mkstemp(buf));
      if (tab_fd < 0) {
        std::cerr << "mkstemp(" << buf << "): " << ::strerror(errno) << std::endl;
        return;
      }
      if (::ftruncate(tab_fd, table_size * sizeof(Bucket)) != 0) {
        std::cerr << "ftruncate: " << ::strerror(errno) << std::endl;
        return;
      }
      tabmap = map_file(tab_fd);
      if (tabmap.first == nullptr) {
        std::cerr << "map_file: " << ::strerror(errno) << std::endl;
        return;
      }
      bucket = static_cast<Bucket *>(tabmap.first);

      //::sprintf(buf, "%s/dat.XXXXXX", dirname.c_str());
      ::sprintf(buf, DAT_TEMP.c_str(), (dirname + HT_DATA).c_str());
      auto dat_fd = FD(::mkstemp(buf));
      if (dat_fd < 0) {
        std::cerr << "mkstemp(" << buf << "): " << ::strerror(errno) << std::endl;
        return;
      }
      if (::ftruncate(dat_fd, table_size*1024) != 0) {
        std::cerr << "ftruncate: " << ::strerror(errno) << std::endl;
        return;
      }
      datmap = map_file(dat_fd);
      if (datmap.first == nullptr) {
        std::cerr << "map_file: " << ::strerror(errno) << std::endl;
        return;
      }
      data = static_cast<char *>(datmap.first);
    }

    std::uint64_t insert(char const *s, std::size_t len, std::uint64_t h)
    {
      for (auto n = h & table_mask; ; n = (n + 1) & table_mask) {
        auto &b = bucket[n];
        if (b.offset == 0u) {
          auto lenptr = reinterpret_cast<std::uint16_t *>(data + datoffs);
          *lenptr = len;
          ::memcpy(lenptr+1, s, len);
          b.offset = datoffs;
          b.hash = h;
          datoffs += sizeof(*lenptr) + *lenptr;
          return b.offset;
        } else if (b.hash == h) {
          auto lenptr = reinterpret_cast<std::uint16_t const *>(data + b.offset);
          if (*lenptr == len && ::memcmp(lenptr+1, s, len) == 0) {
            return b.offset;
          }
        }
      }
    }
  };

  void do_work(std::string const &dir, std::uint64_t which, int core)
  {
    //Table table(dir + "/HT");
    Table table(dir + HT);

    std::random_device rd;
    std::mt19937 rng(rd());
    auto char_dis = std::uniform_int_distribution<char>('A', 'Z');
    auto len_dis = std::uniform_int_distribution<std::size_t>(50, 140);

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


    which <<= 64-5;
    for (int f = 0; f < 100; ++f) {
      /*
      char fname[128];
      ::sprintf(fname, "%s/INPUT/file.%03d", dir.c_str(), f);
      //auto fd = FD(::open(fname, O_RDWR));
      auto fd = FD(::open(fname, O_RDONLY));
      if (fd < 0) {
        std::cerr << "open(" << fname << ": " << ::strerror(errno) << std::endl;
        return;
      }
      auto map = map_file(fd, false);
      if (map.first == nullptr) {
         std::cerr << "map_file: " << ::strerror(errno) << std::endl;
         return;
       }

      // data is just length here
      auto len = static_cast<std::uint16_t *>(map.first);
      auto const len_end = len + map.second;
      */

      // we're going to create the keys on the fly, create the file if it isn't there already
      // and open for write
      char fname[128];
      ::sprintf(fname, IN_FILE_KEY.c_str(), dir.c_str(), f);
      auto keyid_fd = FD(::open(fname,O_RDWR | O_CREAT)); // O_SYNC ?



////////////////////////////////
      char hash_buf[2*1024];
      ::sprintf(hash_buf, IN_FILE_HASH.c_str(), dir.c_str(), f);
      auto hash_fd = FD(::open(hash_buf, O_RDONLY));
      if (hash_fd < 0)
      {
        std::cerr << "open(" << hash_buf << ": " << ::strerror(errno) << std::endl;
        return;
      }

      auto hash_map = map_file(hash_fd, false);
      if (hash_map.first == nullptr) {
        std::cerr << "map_file: " << ::strerror(errno) << std::endl;
        return;
      }
      //auto hash_data = reinterpret_cast<std::uint64_t *>(hash_map.first);
      auto hash_data = reinterpret_cast<rec *>(hash_map.first);

      auto const hash_data_end = hash_data + hash_map.second;
      //auto len = reinterpret_cast<std::uint16_t *>(hash_data+1);
//////////////////////////////////
      char data_buf[2*1024];
      ::sprintf(data_buf, IN_FILE_DATA.c_str(), dir.c_str(), f);
      auto data_fd = FD(::open(data_buf, O_RDONLY));
      if (data_fd < 0)
      {
        std::cerr << "open(" << data_buf << ": " << ::strerror(errno) << std::endl;
        return;
      }

      auto data_map = map_file(data_fd, false);
      if (data_map.first == nullptr) {
        std::cerr << "map_file: " << ::strerror(errno) << std::endl;
        return;
      }
      auto const runon_data = reinterpret_cast<std::uint8_t *>(data_map.first);
      //auto const runon_data_end = runon_data + data_map.second;

      std::uint64_t record_num = 0;

      //std::uint64_t max_records=(key_data_end-key_data)/sizeof(key_data);
      std::uint64_t max_records=(hash_data_end - hash_data)/sizeof(rec);

      std::uint64_t  data_seek = 0;
      while (( record_num < max_records)){/* &&
          (runon_data+data_seek < runon_data_end) && 
          //(key_data+record_num < key_data_end) && 
          (hash_data+record_num < hash_data_end)) {
          //(len+record_num+*(len+record_num) < len_end)) {
        */

        auto hash_ptr = hash_data + record_num;
        if ((hash_ptr->hash & 0xfc00000000000000ul) == which) {
          const char * buf_ptr = reinterpret_cast<const char *>( runon_data  + data_seek);
          auto key = table.insert(buf_ptr, hash_ptr->len, hash_ptr->hash);
          auto written = ::pwrite(keyid_fd, 
              reinterpret_cast<const void *>(&key), sizeof(key), static_cast<off_t>(record_num*sizeof(key)));
          if(written != sizeof(key))
          {
            std::cerr << "keyfile wrote " << written << ":" << sizeof(key) << " bytes." << std::endl;
          }
        }
        data_seek += hash_ptr->len; // size of the string
        ++record_num;
      }
      ::munmap(hash_map.first, hash_map.second);
      ::munmap(data_map.first, data_map.second);
    }
  }

}

int main(int argc, char * argv[])
{
  if (argc != 2) {
    std::cerr << "usage: xxx <path-to-data-files>" << std::endl;
    return 1;
  }
  std::string dir = argv[1];

  auto ncpu = std::thread::hardware_concurrency();
  std::cerr << "hardware_concurrency: " << ncpu << std::endl;



  size_t total_coordinators{64}, total{0};

  std::vector<std::thread> threads;
  while(total < total_coordinators)
  {
    for (uint8_t cpus = 0; (cpus < ncpu) && total < total_coordinators; ++total) {
      threads.emplace_back(do_work, dir, total, cpus++);
    }
    for (auto &t: threads) {
      t.join();
    }
    threads.clear();
  }
  return 0;
}
