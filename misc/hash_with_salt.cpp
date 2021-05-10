#include <iostream>
#include <string>

using namespace std;

class test {};

static int _salt = 0;

int get_index() {
  std::shared_ptr ptr = std::make_shared<test>();
  std::size_t salt = _salt++;
  std::size_t mutex_index = reinterpret_cast<std::size_t>(ptr.get());
  mutex_index += (reinterpret_cast<std::size_t>(ptr.get()) >> 3);
  mutex_index ^= salt + 0x9e3779b9 + (mutex_index << 6) + (mutex_index >> 2);
  mutex_index = mutex_index % 193;
  return mutex_index;
}

int main(int argc, char const *argv[]) {
  for (int i = 0; i < 200; ++i) {
    std::cout << get_index() << std::endl;
  }

  return 0;
}
