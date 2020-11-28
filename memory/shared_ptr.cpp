#include <iostream>

#include "gtest/gtest.h"

class ObjA {
 public:
  ObjA() { std::cout << "ObjA constructing" << std::endl; }
  ~ObjA() { std::cout << "ObjA deconstructing" << std::endl; }
};

class ObjB {
 public:
  ObjB() { std::cout << "ObjB constructing" << std::endl; }
  ~ObjB() { std::cout << "ObjB deconstructing" << std::endl; }
};

TEST(shared_ptr, deconstruct_in_for_loop) {
  for (int i = 0; i < 3; ++i) {
    std::cout << "start of loop" << std::endl;
    auto a = std::make_shared<ObjA>();
    auto b = std::shared_ptr<ObjB>(new ObjB(), [](ObjB *p) {
      std::cout << "deleting shared ptr" << std::endl;
      delete p;
    });
    std::cout << "end of loop" << std::endl;
  }
}
