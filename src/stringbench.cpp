#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include <atomic>
#include <chrono>
#include <iostream>

void test(void* arena, char* start, char* size, char* target);

std::atomic<uint64_t> checksum{0};

struct StringCopyFromCache {
  // This test copies the same string of size size1 to a new string
  // object on the stack.
  std::string x;
  uint64_t size;
  uint64_t pos = 0;
  StringCopyFromCache(uint64_t size1, uint64_t) : x(size1, 'x'), size(size1) {
    // Initialize to some funny values:
    for (uint64_t i = 0; i < size1; ++i) {
      x[i] = '0' + (char)(i % 78);
    }
  }
  uint64_t work(uint64_t) {
    std::string y = x;  // Make a copy
    uint64_t r = y[pos];
    pos += 47;
    while (pos >= size) {
      pos -= size;
    }
    return r;
  }
};

struct MemCpy {
  // This test allocates a memory area of size `size1` bytes and then
  // copies a pseudo random subrange of size `size2` to some other pseudo
  // random place.
  std::vector<char> v;
  uint64_t size1;
  uint64_t size2;
  uint64_t pos1;
  uint64_t pos2;
  MemCpy(uint64_t size1, uint64_t size2)
      : size1(size1), size2(size2), pos1(0), pos2(1234567 % (size1 - size2)) {
    if (size2 * 100 > size1) {
      std::cout << "size1 must be 100x larger than size2!" << std::endl;
      std::abort();
    }
    if (pos2 < size2) {
      // Just to move it away from pos1
      pos2 += 47 * size2;
    }
    v.reserve(size1);
    for (uint64_t i = 0; i < size1; ++i) {
      v.push_back('0' + (char)(i % 78));
    }
  }
  uint64_t work(uint64_t) {
    memcpy(&v[pos2], &v[pos1], size2);
    pos1 += 47153;
    if (pos1 > size1 - size2) {
      pos1 = pos1 % (size1 - size2);
    }
    pos2 += 47153;
    if (pos2 > size1 - size2) {
      pos2 = pos2 % (size1 - size2);
    }
    return v[pos1];
  }
};

void header() {
  std::cout << fmt::format(
      ".-{:^30s}---{:^10s}---{:^10s}---{:^12s}---{:^12s}---{:^12s}-.\n",
      "------------------------------", "----------", "----------",
      "------------", "------------", "------------");
  std::cout << fmt::format(
      "| {:^30s} | {:^10s} | {:^10s} | {:^12s} | {:^12s} | {:^12s} |\n", "test",
      "s1", "s2", "number ops", "total time", "time per op");
  std::cout << fmt::format(
      "|-{:^30s}---{:^10s}---{:^10s}---{:^12s}---{:^12s}---{:^12s}-|\n",
      "------------------------------", "----------", "----------",
      "------------", "------------", "------------");
}

void footer() {
  std::cout << fmt::format(
      "'-{:^30s}---{:^10s}---{:^10s}---{:^12s}---{:^12s}---{:^12s}-'\n",
      "------------------------------", "----------", "----------",
      "------------", "------------", "------------");
}

void report(std::string_view title, uint64_t n, uint64_t s1, uint64_t s2,
            std::chrono::duration<double> runTime) {
  std::cout << fmt::format(
      "> {:<30s} | {:>10d} | {:>10d} | {:>12d} | {:>12%Q%q} | {:>12%Q%q} "
      "|\n",
      title, s1, s2, n, runTime,
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          runTime / static_cast<double>(n)));
}

struct MemSet {
  // This test allocates a memory area of size `size1` bytes and then
  // memsets a pseudo random subrange of size `size2` with some value.
  std::vector<char> v;
  uint64_t size1;
  uint64_t size2;
  uint64_t pos;
  MemSet(uint64_t size1, uint64_t size2) : size1(size1), size2(size2), pos(0) {
    if (size2 * 100 > size1) {
      std::cout << "size1 must be 100x larger than size2!" << std::endl;
      std::abort();
    }
    v.reserve(size1);
    for (uint64_t i = 0; i < size1; ++i) {
      v.push_back('0' + (char)(i % 78));
    }
  }
  uint64_t work(uint64_t i) {
    memset(&v[pos], static_cast<uint8_t>(i), size2);
    pos += 47153;
    if (pos > size1 - size2) {
      pos = pos % (size1 - size2);
    }
    return v[pos];
  }
};

auto testTime = std::chrono::duration<double>(5);
auto warmupTime = std::chrono::duration<double>(1);

template <typename T>
void measure(std::string_view title, uint64_t s1, uint64_t s2) {
  T o(s1, s2);

  // Warmup and time calibration:
  uint64_t dummy = 0;
  uint64_t pos = 0;
  uint64_t count = 0;
  auto warmupStart = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - warmupStart < warmupTime) {
    for (uint64_t i = 0; i < 100; ++i) {
      dummy += o.work(i);
      count++;
    }
  }
  auto warmupEnd = std::chrono::steady_clock::now();
  std::chrono::duration<double> warmupMeasured = warmupEnd - warmupStart;
  uint64_t n = count * testTime.count() / warmupMeasured.count();

  // Actual run:
  auto start = std::chrono::steady_clock::now();
  for (uint64_t i = 0; i < n; ++i) {
    dummy += o.work(i);
  }
  auto end = std::chrono::steady_clock::now();

  checksum += dummy;  // Avoid optimizations
  report(title, n, s1, s2, end - start);
}

int main(int argc, char* argv[]) {
  std::cout << "Hello, I measure performance of string and memory operations."
            << std::endl;
  if (argc > 1) {
    testTime = std::chrono::duration<double>(atol(argv[1]));
  }
  if (argc > 2) {
    warmupTime = std::chrono::duration<double>(atol(argv[2]));
  }

  header();

  // Copy of a string:
  measure<StringCopyFromCache>("string_copy_from_cache", 32, 0);
  measure<StringCopyFromCache>("string_copy_from_cache", 80, 0);
  measure<StringCopyFromCache>("string_copy_from_cache", 257, 0);
  measure<StringCopyFromCache>("string_copy_from_cache", 1234, 0);
  measure<StringCopyFromCache>("string_copy_from_cache", 1024 * 1024 + 17, 0);

  measure<MemCpy>("memcpy", 1024 * 1024 + 17, 32);
  measure<MemCpy>("memcpy", 1024 * 1024 + 17, 80);
  measure<MemCpy>("memcpy", 1024 * 1024 + 17, 257);
  measure<MemCpy>("memcpy", 1024 * 1024 + 17, 1234);
  measure<MemCpy>("memcpy", 1024 * 1024 * 1024 + 17, 32);
  measure<MemCpy>("memcpy", 1024 * 1024 * 1024 + 17, 1234);
  measure<MemCpy>("memcpy", 1024 * 1024 * 1024 + 17, 40763);

  measure<MemSet>("memset", 1024 * 1024 + 17, 32);
  measure<MemSet>("memset", 1024 * 1024 + 17, 80);
  measure<MemSet>("memset", 1024 * 1024 + 17, 257);
  measure<MemSet>("memset", 1024 * 1024 + 17, 1234);
  measure<MemSet>("memset", 1024 * 1024 * 1024 + 17, 32);
  measure<MemSet>("memset", 1024 * 1024 * 1024 + 17, 1234);
  measure<MemSet>("memset", 1024 * 1024 * 1024 + 17, 40763);

  footer();

  return 0;
}
