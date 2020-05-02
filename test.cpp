#include <iostream>
#include <string>

#include "ThreadPool.h"

using namespace std;

int main(int argc, char const *argv[]) {
  ThreadPool tp(1);
  std::string s("111");
  cout << s.size() << endl;
  return 0;
}
