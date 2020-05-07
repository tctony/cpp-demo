#include <iostream>
#include <string>
#include <vector>

#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"

void str_join_split() {
  std::vector<std::string> v = {"foo", "bar", "baz"};

  auto s = absl::StrJoin(v, "-");
  std::cout << "Joined string: " << s << "\n";

  struct not_foo {
    bool operator()(absl::string_view str) { return "foo" != str; }
  };
  auto parts = absl::StrSplit(s, absl::MaxSplits('-', 1), not_foo());
  std::cout << "Splited string:\n";
  for (auto str : parts) {
    std::cout << str << "\n";
  }
}

void str_cat_append() {
  auto cat_str = absl::StrCat("foo", "bar", "baz", 123, 0.0000001,
                              absl::Hex(255, absl::kZeroPad4), false);
  std::cout << "cated string: " << cat_str << "\n";

  std::string append_str;
  absl::StrAppend(&append_str, "foo", "bar", "baz", 123, 0.0000001,
                  absl::Hex(255, absl::kZeroPad4), false);
  std::cout << "appended string: " << append_str << "\n";
}

int main() {
  str_join_split();

  str_cat_append();

  std::cout << absl::StrFormat("Welcome to %s, Number %d!\n", "The Village", 6);

  return (0);
}