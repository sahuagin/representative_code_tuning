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
    map_file(int fd)
    {
      std::pair<void *, std::size_t> result{nullptr, 0u};
      struct stat status;
      if (::fstat(fd, &status) != 0) {
        return result;
      }
      auto filesz = std::size_t(status.st_size);
#if defined(__linux__)
      constexpr int map_flags = MAP_SHARED;
#else
      constexpr int map_flags = MAP_SHARED | MAP_NOCORE;
#endif
      auto addr = ::mmap(nullptr, filesz, PROT_READ|PROT_WRITE, map_flags, fd, 0);
      if (addr == MAP_FAILED) {
        return result;
      }
#if defined(__linux__)
      if (::madvise(addr, filesz, MADV_DONTDUMP) != 0) {
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

  void do_work(std::string const &dir, std::uint64_t which)
  {
    Table table(dir + "/HT");

    //char buf[1024];
    std::random_device rd;
    std::mt19937 rng(rd());
    auto char_dis = std::uniform_int_distribution<char>('A', 'Z');
    auto len_dis = std::uniform_int_distribution<std::size_t>(50, 140);

    which <<= 64-5;
    for (int f = 0; f < 100; ++f) {
      char fname[128];
      ::sprintf(fname, "%s/INPUT/file.%03d", dir.c_str(), f);
      auto fd = FD(::open(fname, O_RDWR));
      if (fd < 0) {
        std::cerr << "open: " << ::strerror(errno) << std::endl;
        return;
      }
      auto map = map_file(fd);
      if (map.first == nullptr) {
        std::cerr << "map_file: " << ::strerror(errno) << std::endl;
        return;
      }
      auto data = static_cast<std::uint8_t *>(map.first);
      auto const data_end = data + map.second;

      while (data < data_end) {
        auto key_ptr = reinterpret_cast<std::uint64_t *>(data);
        auto hash_ptr = key_ptr + 1;
        auto len_ptr = reinterpret_cast<std::uint16_t *>(hash_ptr + 1);
        auto buf_ptr = reinterpret_cast<char *>(len_ptr + 1);
        if ((*hash_ptr & 0xfc00000000000000ul) == which) {
          *key_ptr = table.insert(buf_ptr, *len_ptr, *hash_ptr);
        }
        data = reinterpret_cast<std::uint8_t *>(buf_ptr + *len_ptr);
      }
      ::munmap(map.first, map.second);
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
  std::vector<std::thread> threads;
  for (int i = 0; i < 64; ++i) {
    threads.emplace_back(do_work, dir, i);
  }
  for (auto &t: threads) {
    t.join();
  }
  return 0;
}
