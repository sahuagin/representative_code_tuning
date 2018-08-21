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
    map_file(int fd, bool write=true)
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
      if (::madvise(addr, filesz, MADV_RANDOM) != 0) {
        return result;
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

      ::sprintf(buf, "%s/tab.XXXXXX", dirname.c_str());
      auto fd = FD(::mkstemp(buf));
      if (fd < 0) {
        std::cerr << "mkstemp: " << ::strerror(errno) << std::endl;
        return;
      }
      if (::ftruncate(fd, table_size * sizeof(Bucket)) != 0) {
        std::cerr << "ftruncate: " << ::strerror(errno) << std::endl;
        return;
      }
      tabmap = map_file(fd);
      if (tabmap.first == nullptr) {
        std::cerr << "map_file: " << ::strerror(errno) << std::endl;
        return;
      }
      bucket = static_cast<Bucket *>(tabmap.first);

      ::sprintf(buf, "%s/dat.XXXXXX", dirname.c_str());
      fd = FD(::mkstemp(buf));
      if (fd < 0) {
        std::cerr << "mkstemp: " << ::strerror(errno) << std::endl;
        return;
      }
      if (::ftruncate(fd, table_size*1024) != 0) {
        std::cerr << "ftruncate: " << ::strerror(errno) << std::endl;
        return;
      }
      datmap = map_file(fd);
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
    Table table(dir + "/HT");

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

      auto data = static_cast<std::uint8_t *>(map.first);
      auto const data_end = data + map.second;



      char buf[2*1024];
      ::sprintf(buf, "%s/INPUT/file_key.%03d", dir.c_str(), f);
      auto key_fd = FD(::open(buf, O_RDWR));
      if (key_fd < 0)
      {
        std::cerr << "open(" << buf << ": " << ::strerror(errno) << std::endl;
        return;
      }

      auto key_map = map_file(key_fd, true);
      if (key_map.first == nullptr) {
        std::cerr << "map_file: " << ::strerror(errno) << std::endl;
        return;
      }
      auto key_data = reinterpret_cast<std::uint64_t *>(key_map.first);
      auto const key_data_end = key_data + key_map.second;
////////////////////////////////
      char hash_buf[2*1024];
      ::sprintf(hash_buf, "%s/INPUT/file_hash.%03d", dir.c_str(), f);
      auto hash_fd = FD(::open(hash_buf, O_RDWR));
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
      auto hash_data = reinterpret_cast<std::uint64_t *>(hash_map.first);
      auto const hash_data_end = hash_data + hash_map.second;
//////////////////////////////////

      std::ptrdiff_t record_num = 0;
      while ((data < data_end) && (key_data < key_data_end) && (hash_data < hash_data_end)) {
        auto key_ptr = reinterpret_cast<std::uint64_t *>(key_data+record_num);

        //auto hash_ptr = hash_data++;
        auto hash_ptr = hash_data + record_num;
        auto len_ptr = reinterpret_cast<std::uint16_t *>(data+record_num);
        auto buf_ptr = reinterpret_cast<char *>(len_ptr + 1);
        if ((*hash_ptr & 0xfc00000000000000ul) == which) {
          *key_ptr = table.insert(buf_ptr, *len_ptr, *hash_ptr);
          //++key_ptr;
        }
        ++record_num;
        data = reinterpret_cast<std::uint8_t *>(buf_ptr + *len_ptr);
        //++key_ptr;
      }
      ::munmap(map.first, map.second);
      ::munmap(key_map.first, key_map.second);
      ::munmap(hash_map.first, hash_map.second);
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
