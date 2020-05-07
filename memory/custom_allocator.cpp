#include <iostream>
#include <memory>
#include <vector>

class Object {};

template <typename T>
struct MyAllocator {
  using value_type = T;

  static T *allocate(std::size_t size) {
    void *p = std::malloc(size);
    return reinterpret_cast<T *>(p);
  }

  static void deallocate(T *p, std::size_t size) {
    if (p != nullptr) std::free(p);
  }
};

int main(int argc, char const *argv[]) {
  std::vector<Object, MyAllocator<Object>> v;
  for (int i = 0; i < 100; ++i) {
    v.push_back(Object());
    v.emplace_back();
  }
  return 0;
}
