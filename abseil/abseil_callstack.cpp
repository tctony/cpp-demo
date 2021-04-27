#include <iostream>
#include <exception>

#include "base/util/callstack.h"

void testCallStack() {
  auto callstack = base::util::CallStack(0);
  std::cout << callstack.debugString() << "\n";
}

struct MyException: public std::exception {
  const char* what() const throw() {
    return "MyExcetpion";
  }
};

void testExceptionCallStack() {
  try {
    throw MyException();
  } catch(MyException& exception) {
    std::cout << exception.what() << "\n";
    std::cout 
      << base::util::CallStack::currentExceptionCallstack.get()->debugString()
      << "\n";
  }
}

int main(int argc, char const *argv[])
{
  testCallStack();

  testExceptionCallStack();

  return 0;
}
