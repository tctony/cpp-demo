#include <forward_list>
#include <iostream>
#include <map>
#include <unordered_map>

#include "gtest/gtest.h"

TEST(map, unordered_multimap_find) {
  std::unordered_multimap<int, int> m;
  m.insert(std::make_pair(1, 11));
  m.insert(std::make_pair(1, 12));
  m.insert(std::make_pair(1, 13));
  m.insert(std::make_pair(2, 21));
  m.insert(std::make_pair(2, 22));

  for (auto k : m) {
    std::cout << k.first << ": " << k.second << std::endl;
  }

  std::cout << std::endl;

  auto itr = m.find(2);
  while (itr != m.end() && itr->first == 2) {
    std::cout << itr->first << ": " << itr->second << std::endl;
    ++itr;
  }
}

TEST(forward_list, push_and_pop) {
  std::forward_list<int> list;
  for (int i = 0; i < 100; ++i) {
    list.push_front(10);
  }
  while (!list.empty()) {
    list.pop_front();
  }
}

template <typename T>
std::ostream& operator<<(std::ostream& s, const std::forward_list<T>& v) {
  s.put('[');
  char comma[3] = {'\0', ' ', '\0'};
  for (const auto& e : v) {
    s << comma << e;
    comma[0] = ',';
  }
  return s << ']';
}

TEST(forward_list, insert_after_end) {
  std::forward_list<int> list;
  // will crash
  // list.insert_after(list.begin(), 2);
  std::cout << list << std::endl;
}