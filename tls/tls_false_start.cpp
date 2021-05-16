// clang-format off
#define ZLOG_TAG "zlog"
#include "base/zlog/zlog_to_console.h"
// clang-format on

#include <err.h>
#include <netdb.h>

#include <chrono>
#include <iostream>
#include <thread>

#include <openssl/ssl.h>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"
#include "base/http/request_formatter.hpp"
#include "base/http/response_parser.hpp"
#include "base/util/time_util.hpp"
#include "demo/common/abseil_flag_ipport.hpp"

#define N_LOOP N_TRY
#ifndef N_TRY
#define N_TRY 3
#endif

#define GET_SSL_ERR() ""

ABSL_FLAG(int, optimize_level, 3,
          "0: no optimize; 1: session resumption; 2: session resumption and "
          "false start");
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
    int ret = close(*ptr);
    absl::PrintF("close socket ret: %d\n", ret);
    delete ptr;
  });
}

std::shared_ptr<SSL> create_ssl_conn(std::shared_ptr<SSL_CTX> ctx,
                                     std::shared_ptr<SSL_SESSION> *session_ptr,
                                     std::string sni,
                                     std::shared_ptr<int> tcp_conn) {
  std::shared_ptr<SSL> ssl(SSL_new(ctx.get()), [](SSL *ptr) {
    auto ret = SSL_shutdown(ptr);
    // 发送close_notify
    if (ret == 0) {
      // 接收remote close_notify
      ret = SSL_shutdown(ptr);
      if (ret != 1) {
        absl::PrintF("ssl shutdown(2) incorrectly:%d %s\n", ret, GET_SSL_ERR());
      }
    } else if (ret != 1) {
      absl::PrintF("ssl shutdown(1) incorrectly:%d %s\n", ret, GET_SSL_ERR());
    }
    SSL_free(ptr);
  });
  if (!ssl) {
    absl::PrintF("unable to create ssl struct. %s\n", GET_SSL_ERR());
    return nullptr;
  }

  assert(tcp_conn != nullptr);
  SSL_set_fd(ssl.get(), *(tcp_conn.get()));

  if (SSL_set_tlsext_host_name(ssl.get(), sni.c_str()) != 1) {
    absl::PrintF("unable to set tls sni: %s\n", sni);
    return nullptr;
  }

  // SSL_set_options(ssl.get(), SSL_OP_NO_TICKET);

  assert(session_ptr != nullptr);
  auto session = *session_ptr;
  if (absl::GetFlag(FLAGS_optimize_level) >= 1 && session) {
    if (SSL_set_session(ssl.get(), session.get()) != 1) {
      absl::PrintF("unable to set reuse session. %s\n", GET_SSL_ERR());
      return nullptr;
    } else {
      // TODO if session ticket out live its lifetime, we should enable false
      // start to achieve better performance
      zinfo("reusing ssl session\n");
    }
  } else {
    zinfo("no session usable");
  }

  if (SSL_connect(ssl.get()) != 1) {
    absl::PrintF("unable to do ssl handshake with %s. %s\n", sni,
                 GET_SSL_ERR());
    return nullptr;
  }

  return ssl;
}

void do_http_over_ssl(std::shared_ptr<SSL> ssl, std::string sni) {
  base::http::Request request("/cloudim/cloudxw-bin/xwtext");
  request.headerList.add("Host", sni);
  request.headerList.add("Connection", "keep-alive");
  std::string request_buffer = base::http::RequestFormatter::format(request);

  int nw =
      SSL_write(ssl.get(), request_buffer.c_str(), request_buffer.length());
  if (nw != request_buffer.length()) {
    absl::PrintF("ssl write %s\n", GET_SSL_ERR());
    return;
  }

  absl::PrintF("https response start---\n");

  base::http::Response response;
  auto responseParser = base::http::ResponseParser(response);
  do {
    std::array<char, 2048> buffer;
    int nr = SSL_read(ssl.get(), buffer.data(), buffer.size());
    if (nr <= 0) {
      absl::PrintF("ssl read %s\n", GET_SSL_ERR());
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

static void InfoCallback(const SSL *ssl, int type, int value) {
  switch (type) {
  case SSL_CB_HANDSHAKE_START:
    zinfo("Handshake started");
    break;
  case SSL_CB_HANDSHAKE_DONE:
    zinfo("Handshake done");
    break;
  case SSL_CB_CONNECT_LOOP:
    zinfo("Handshake progress: %_", SSL_state_string_long(ssl));
    break;
  }
}

int main(int argc, char *argv[]) {
  absl::ParseCommandLine(argc, argv);

  auto sni = absl::GetFlag(FLAGS_sni);
  auto ip_ports = absl::GetFlag(FLAGS_ip_ports);

  if (sni.length() == 0) {
    zerror("sni not set, see --help");
    exit(-1);
  } else if (ip_ports.size() == 0) {
    zerror("ip_ports not set, see --help");
    exit(-1);
  } else {
    zinfo("checking %_ on %_ with optimize level %_", sni,
          AbslUnparseFlag(ip_ports), absl::GetFlag(FLAGS_optimize_level));
  }

  std::shared_ptr<SSL_CTX> ctx(SSL_CTX_new(TLS_client_method()), SSL_CTX_free);
  if (ctx == nullptr) {
    zerror("unable to init ssl context. %_", GET_SSL_ERR());
    exit(-1);
  }

  if (absl::GetFlag(FLAGS_optimize_level) >= 2) {
    zinfo("enable false start");
    SSL_CTX_set_mode(ctx.get(), SSL_MODE_ENABLE_FALSE_START);
    SSL_CTX_set_false_start_allowed_without_alpn(ctx.get(), 1);
  }

  SSL_CTX_set_info_callback(ctx.get(), InfoCallback);

  std::shared_ptr<SSL_SESSION> session;

  for (int i = 0; i < N_LOOP; ++i) {
    for (auto target : ip_ports) {
      auto tcp_conn_clock = base::util::SimpleClock("tcp_conn_cost");
      auto tcp = create_tcp_conn(target.ip, target.port);
      assert(tcp != nullptr);
      zinfo("tcp conn cost %_ms", tcp_conn_clock.end());

      auto tls_conn_clock = base::util::SimpleClock("tls_conn_cost");
      auto ssl = create_ssl_conn(ctx, &session, sni, tcp);
      assert(ssl != nullptr);
      zinfo("tls conn cost %_ms", tls_conn_clock.end());
      zinfo("tls conn false start %_", SSL_in_false_start(ssl.get()));

      for (int j = 0; j < N_TRY; ++j) {
        auto http_clock = base::util::SimpleClock("http_cost");
        do_http_over_ssl(ssl, sni);
        zinfo("http write read cost %_ms", http_clock.end());

        std::cout << std::endl;

        if (j)
          continue;
        // if we do false start, we might get reusable session until now
        auto ssl_session = SSL_get1_session(ssl.get());
        if (ssl_session) {
          session.reset(ssl_session,
                        [](SSL_SESSION *ptr) { SSL_SESSION_free(ptr); });

          if (absl::GetFlag(FLAGS_optimize_level) >= 2) {
            zinfo("disable false start");
            SSL_CTX_clear_mode(ctx.get(), SSL_MODE_ENABLE_FALSE_START);
            SSL_CTX_set_false_start_allowed_without_alpn(ctx.get(), 0);
          }
        }
      }

      using namespace std::chrono_literals;
      // std::this_thread::sleep_for(100ms);
    }

    using namespace std::chrono_literals;
    // std::this_thread::sleep_for(5min + 10s);
  }

  return 0;
}
