#if !defined(COMMON_ABSEIL_FLAG_IPPORT_HPP)
#define COMMON_ABSEIL_FLAG_IPPORT_HPP

#include "absl/strings/str_join.h"

// namespace common {

struct IpPort {
  explicit IpPort(std::string i, std::string p) : ip(i), port(p) {}
  std::string ip;
  std::string port;  // valid range is [0..32767]

  operator std::string() const { return absl::StrCat(ip, ":", port); }
};

//}  // namespace common

extern std::string AbslUnparseFlag(std::vector<IpPort> ip_ports);

extern bool AbslParseFlag(absl::string_view text, std::vector<IpPort> *ip_ports,
                          std::string *error);

#endif  // COMMON_ABSEIL_FLAG_IPPORT_HPP
