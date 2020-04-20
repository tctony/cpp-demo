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