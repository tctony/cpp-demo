#include <iostream>
#include <thread>

int main(int argc, char const *argv[]) {
  unsigned int n = std::thread::hardware_concurrency();
  std::cout << n << " concurrent threads are supported.\n";
  return 0;
}
