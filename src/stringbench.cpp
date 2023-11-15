#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <string.h>

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

struct StringCopy {
  // This test creates size1 random strings of size size2 and then copies
  // random ones over others.
  std::vector<std::string> v;
  uint64_t size1;
  uint64_t size2;
  uint64_t pos1;
  uint64_t pos2;
  StringCopy(uint64_t size1, uint64_t size2)
      : size1(size1), size2(size2), pos1(0), pos2(1234567 % size1) {
    v.reserve(size1);
    for (size_t i = 0; i < size1; ++i) {
      std::string s(size2, 'x');
      for (size_t j = 0; j < size2; ++j) {
        s[j] = static_cast<char>((i * size2 + j) % 256);
      }
      v.push_back(std::move(s));
    }
  }
  uint64_t work(uint64_t) {
    v[pos1] = v[pos2];
    pos1 += 12341;
    if (pos1 >= size1) {
      pos1 = pos1 % size1;
    }
    pos2 += 24123;
    if (pos2 >= size1) {
      pos2 = pos2 % size1;
    }
    return v[pos1][0];
  }
};

struct Substr {
  // This test allocates a memory area of size `size1` bytes in a string
  // and then copies a pseudo random subrange of size `size2` to some
  // other string variable using the substr method.
  std::string s;
  uint64_t size1;
  uint64_t size2;
  uint64_t pos;
  std::string sub;
  Substr(uint64_t size1, uint64_t size2) : size1(size1), size2(size2), pos(0) {
    s.reserve(size1);
    for (uint64_t i = 0; i < size1; ++i) {
      s.push_back('0' + (char)(i % 78));
    }
  }
  uint64_t work(uint64_t) {
    sub = s.substr(pos, size2);
    pos += 47153;
    if (pos > size1 - size2) {
      pos = pos % (size1 - size2);
    }
    return s[pos];
  }
};

struct StrLen {
  // This test creates size1 random strings of size size2 and then computes
  // strlen of random ones.
  std::vector<std::string> v;
  uint64_t size1;
  uint64_t size2;
  uint64_t pos1;
  StrLen(uint64_t size1, uint64_t size2) : size1(size1), size2(size2), pos1(0) {
    v.reserve(size1);
    for (size_t i = 0; i < size1; ++i) {
      std::string s(size2 + (i % 8), 'x');
      for (size_t j = 0; j < size2; ++j) {
        s[j] = static_cast<char>(((i * size2 + j) % 255) + 1);
      }
      v.push_back(std::move(s));
    }
  }
  uint64_t work(uint64_t) {
    uint64_t res = strlen(v[pos1].c_str());
    pos1 += 12341;
    if (pos1 >= size1) {
      pos1 = pos1 % size1;
    }
    return res;
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

struct MemMove {
  // This test allocates a memory area of size `size1` bytes and then
  // copies a pseudo random subrange of size `size2` to some other pseudo
  // random place using memmove, regions can overlap.
  std::vector<char> v;
  uint64_t size1;
  uint64_t size2;
  uint64_t pos1;
  uint64_t pos2;
  MemMove(uint64_t size1, uint64_t size2)
      : size1(size1), size2(size2), pos1(0), pos2(1234567 % (size1 - size2)) {
    if (size2 * 100 > size1) {
      std::cout << "size1 must be 100x larger than size2!" << std::endl;
      std::abort();
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
    pos2 += 97251;
    if (pos2 > size1 - size2) {
      pos2 = pos2 % (size1 - size2);
    }
    return v[pos1];
  }
};

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

void divider() {
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

  measure<StringCopyFromCache>("string_copy_from_cache", 32, 0);
  measure<StringCopyFromCache>("string_copy_from_cache", 80, 0);
  measure<StringCopyFromCache>("string_copy_from_cache", 257, 0);
  measure<StringCopyFromCache>("string_copy_from_cache", 1234, 0);
  measure<StringCopyFromCache>("string_copy_from_cache", 1024 * 1024 + 17, 0);

  divider();

  measure<StringCopy>("string_copy", 100001, 32);
  measure<StringCopy>("string_copy", 100001, 80);
  measure<StringCopy>("string_copy", 100001, 257);
  measure<StringCopy>("string_copy", 10001, 1234);
  measure<StringCopy>("string_copy", 1001, 1024 * 1024 + 17);

  divider();

  measure<Substr>("substr", 100001, 32);
  measure<Substr>("substr", 100001, 80);
  measure<Substr>("substr", 100001, 257);
  measure<Substr>("substr", 1024 * 1024 * 1024 + 17, 48);
  measure<Substr>("substr", 1024 * 1024 * 1024 + 17, 1234);
  measure<Substr>("substr", 1024 * 1024 * 1024 + 17, 1626361);

  divider();

  measure<StrLen>("strlen", 100001, 32);
  measure<StrLen>("strlen", 100001, 80);
  measure<StrLen>("strlen", 100001, 257);
  measure<StrLen>("strlen", 10001, 1234);
  measure<StrLen>("strlen", 1001, 1024 * 1024 + 17);

  divider();

  measure<MemCpy>("memcpy", 1024 * 1024 + 17, 32);
  measure<MemCpy>("memcpy", 1024 * 1024 + 17, 80);
  measure<MemCpy>("memcpy", 1024 * 1024 + 17, 257);
  measure<MemCpy>("memcpy", 1024 * 1024 + 17, 1234);
  measure<MemCpy>("memcpy", 1024 * 1024 * 1024 + 17, 32);
  measure<MemCpy>("memcpy", 1024 * 1024 * 1024 + 17, 1234);
  measure<MemCpy>("memcpy", 1024 * 1024 * 1024 + 17, 40763);

  divider();

  measure<MemMove>("memmove", 1024 * 1024 + 17, 32);
  measure<MemMove>("memmove", 1024 * 1024 + 17, 80);
  measure<MemMove>("memmove", 1024 * 1024 + 17, 257);
  measure<MemMove>("memmove", 1024 * 1024 + 17, 1234);
  measure<MemMove>("memmove", 1024 * 1024 * 1024 + 17, 32);
  measure<MemMove>("memmove", 1024 * 1024 * 1024 + 17, 1234);
  measure<MemMove>("memmove", 1024 * 1024 * 1024 + 17, 40763);

  divider();

  measure<MemSet>("memset", 1024 * 1024 + 17, 32);
  measure<MemSet>("memset", 1024 * 1024 + 17, 80);
  measure<MemSet>("memset", 1024 * 1024 + 17, 257);
  measure<MemSet>("memset", 1024 * 1024 + 17, 1234);
  measure<MemSet>("memset", 1024 * 1024 * 1024 + 17, 32);
  measure<MemSet>("memset", 1024 * 1024 * 1024 + 17, 1234);
  measure<MemSet>("memset", 1024 * 1024 * 1024 + 17, 40763);

  footer();

  std::cout << R"(Explanations:
    string_copy_from_cache:
        Copies a single fixed string of length s1 to a new string object
        on the stack, probably all in cache.
    string_copy:
        Creates s1 strings of length s2 and then copies a random one to another
        random one.
    substr:
        Creates a string of length s1 and then uses substr to copy substrings
        of length s2 out to a string variable.
    strlen:
        Creates s1 strings of length s2 and then uses strlen on random ones.
    memcpy:
        Creates a memory area of size s1 and then uses memcpy to copy some random
        area of size s2 to another one (non-overlapping).
    memmove:
        Creates a memory area of size s1 and then uses memmove to copy some random
        area of size s2 to another one (possibly overlapping).
    memset:
        Creates a memory area of size s1 and then uses memset to set some random area
        of size s2 to a random byte value.
)";
  return 0;
}
