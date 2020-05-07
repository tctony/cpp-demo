#include <iostream>

#include "util/time_util.hpp"

using namespace std;

static void* m = nullptr;

class student {
  string name;
  int age;

 public:
  student(string name, int age) {
    this->name = name;
    this->age = age;
  }
  void display() {
    // cout << "Name:" << name << endl;
    // cout << "Age:" << age << endl;
  }
  void* operator new(size_t size) {
    if (m == nullptr) {
      m = malloc(size);
    }
    return m;
  }

  void operator delete(void* p) {
    // free(p);
  }
};

int main() {
  auto cost = base::util::timeCostInMilliseconds([] {
    for (int i = 0; i < 1000000; ++i) {
      student* p = new student("yash", 24);
      p->display();
      delete p;
    }
  });
  std::cout << "cost: " << cost << " ms\n";
}