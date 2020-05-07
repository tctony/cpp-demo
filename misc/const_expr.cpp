#include <chrono>
#include <iostream>

#define N 25

uint64_t f1(int n) { return n == 1 || n == 2 ? 1 : f1(n - 1) + f1(n - 2); }

constexpr uint64_t f2(const int n) {
  return n == 1 || n == 2 ? 1 : f2(n - 1) + f2(n - 2);
}

int main(int argc, const char *argv[]) {
  {
    auto ts = std::chrono::high_resolution_clock::now();
    uint64_t v = 0;
    for (int i = 0; i < 100; ++i) {
      v += f1(N);
    }
    auto te = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(te - ts).count();
    std::cout << duration << std::endl;
  }

  {
    auto ts = std::chrono::high_resolution_clock::now();
    uint64_t v = 0;
    for (int i = 0; i < 100; ++i) {
      v += f2(N);
    }
    auto te = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(te - ts).count();
    std::cout << duration << std::endl;
  }

  {
    auto ts = std::chrono::high_resolution_clock::now();
    uint64_t v = 0;
    constexpr uint64_t vv = f2(N);
    for (int i = 0; i < 100; ++i) {
      v += vv;
    }
    auto te = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(te - ts).count();
    std::cout << duration << std::endl;
  }

  return 0;
}
