#include <regex>

#include "gtest/gtest.h"

TEST(regex, match) {
  std::string fnames[] = {"foo.txt", "bar.txt", "test", "a0.txt", "AAA.txt"};
  std::regex txt_regex("[a-z]+\\.txt");
  for (const auto &fname : fnames) {
    std::cout << fname << ": " << std::regex_match(fname, txt_regex)
              << std::endl;
  }
}

TEST(regex, smatch) {
  std::string fnames[] = {"foo.txt", "bar.txt", "test", "a0.txt", "AAA.txt"};

  std::regex base_regex("([a-z]+)\\.txt");
  std::smatch base_match;

  for (const auto &fname : fnames) {
    if (std::regex_match(fname, base_match, base_regex)) {
      // std::smatch 的第一个元素匹配整个字符串
      // std::smatch 的第二个元素匹配了第一个括号表达式
      if (base_match.size() == 2) {
        std::cout << "sub-match[0]: " << base_match[0].str() << std::endl;
        std::string base = base_match[1].str();
        std::cout << fname << " sub-match[1]: " << base << std::endl;
      }
    }
  }
}
