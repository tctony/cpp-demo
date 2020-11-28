#include <err.h>
#include <netdb.h>

#include <iostream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"
#include "base/http/request_formatter.hpp"
#include "base/http/response_parser.hpp"
#include "base/util/time_util.hpp"
#include "demo/common/abseil_flag_ipport.hpp"

#define N_LOOP 3
#define N_TRY 5

ABSL_FLAG(std::string, sni, "", "sni name of https cert");
ABSL_FLAG(std::vector<IpPort>, ip_ports, std::vector<IpPort>(),
          "pairs of ip:port");

std::shared_ptr<int> create_tcp_conn(std::string ip, std::string port) {
  struct addrinfo hints, *ai = nullptr;
  int err, fd;

  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = 0;
  hints.ai_family = PF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  err = getaddrinfo(ip.c_str(), port.c_str(), &hints, &ai);
  if (err != 0) {
    absl::PrintF("unable to solve '%s:%s': %s", ip, port, gai_strerror(err));
    return nullptr;
  }

  assert(ai != nullptr);
  std::shared_ptr<addrinfo> addr(ai, free);

  fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
  if (fd == -1) {
    absl::PrintF("unable to create socket for '%s:%s'\n", ip, port);
    return nullptr;
  }

  err = connect(fd, addr->ai_addr, addr->ai_addrlen);
  if (err == -1) {
    absl::PrintF("unable to connect to '%s:%s'\n", ip, port);
    return nullptr;
  }

  assert(fd > 0);
  int *fd_ptr = new int(fd);
  return std::shared_ptr<int>(fd_ptr, [](int *ptr) {
    close(*ptr);
    delete ptr;
  });
}

void do_http(std::shared_ptr<int> tcp, std::string sni) {
  base::http::Request request("/test_json");
  request.headerList.add("Host", sni);
  request.headerList.add("Connection", "keep-alive");
  std::string request_buffer = base::http::RequestFormatter::format(request);
  int nw = write(*(tcp.get()), request_buffer.c_str(), request_buffer.length());
  if (nw != request_buffer.length()) {
    absl::PrintF("tcp write %d/%d\n", nw, request_buffer.length());
    return;
  }

  absl::PrintF("https response start---\n");

  base::http::Response response;
  auto responseParser = base::http::ResponseParser(response);
  do {
    std::array<char, 2048> buffer;
    int nr = read(*(tcp.get()), buffer.data(), buffer.size());
    if (nr <= 0) {
      absl::PrintF("ssl read %d\n", nr);
      return;
    }
    absl::PrintF("%s", std::string(buffer.data(), nr));
    auto [status, offset] =
        responseParser.parse(buffer.data(), buffer.data() + nr);
    if (status == base::http::ResponseParser::good) {
      absl::PrintF("---https response end\n");
      return;
    }
    if (status == base::http::ResponseParser::indeterminate) {
      continue;
    }
    absl::PrintF("invalid http response\n");
    return;
  } while (true);
}

int main(int argc, char *argv[]) {
  absl::ParseCommandLine(argc, argv);

  auto sni = absl::GetFlag(FLAGS_sni);
  auto ip_ports = absl::GetFlag(FLAGS_ip_ports);

  if (sni.length() == 0) {
    absl::PrintF("sni not set, see --help\n");
    exit(-1);
  } else if (ip_ports.size() == 0) {
    absl::PrintF("ip_ports not set, see --help\n");
    exit(-1);
  } else {
    absl::PrintF("testing tcp reuse for %s at %s\n", sni,
                 AbslUnparseFlag(ip_ports));
  }

  for (int i = 0; i < N_LOOP; ++i) {
    for (auto target : ip_ports) {
      auto tpc_conn_clock = base::util::SimpleClock("tcp_conn_cost");
      auto tcp = create_tcp_conn(target.ip, target.port);
      assert(tcp != nullptr);
      absl::PrintF("new tcp conn cost %lldms\n", tpc_conn_clock.end());

      for (int j = 0; j < N_TRY; ++j) {
        auto http_clock = base::util::SimpleClock("http_cost");
        do_http(tcp, sni);
        absl::PrintF("http write read cost %lldms\n", http_clock.end());
      }
    }
  }

  return 0;
}
