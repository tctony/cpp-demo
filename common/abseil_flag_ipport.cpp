#include "abseil_flag_ipport.hpp"

#include "absl/flags/marshalling.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"

std::string AbslUnparseFlag(std::vector<IpPort> ip_ports) {
  return absl::StrJoin(ip_ports, ",",
                       [](std::string *out, const IpPort &ipport) {
                         absl::StrAppend(out, std::string(ipport));
                       });
}

bool AbslParseFlag(absl::string_view text, std::vector<IpPort> *ip_ports,
                   std::string *error) {
  if (text.length() == 0) return true;
  std::vector<absl::string_view> pieces = absl::StrSplit(text, ",");
  for (auto p : pieces) {
    std::vector<absl::string_view> pair = absl::StrSplit(p, ":");
    if (pair.size() != 2) {
      *error = absl::StrCat("invalid ipport: ", p);
      return false;
    }
    int port;
    if (!absl::ParseFlag(pair[1], &port, error)) {
      return false;
    }
    if (port < 0 || port > 32767) {
      *error = absl::StrCat("invalid port: ", pair[1]);
      return false;
    }
    ip_ports->emplace_back(std::string(pair[0]), std::string(pair[1]));
  }
  return true;
}
