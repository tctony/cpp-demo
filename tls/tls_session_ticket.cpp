#include <err.h>
#include <netdb.h>

#include <iostream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "base/http/request_formatter.hpp"
#include "base/http/response_parser.hpp"
#include "base/util/time_util.hpp"
#include "demo/common/abseil_flag_ipport.hpp"
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#define N_LOOP N_TRY
#ifndef N_TRY
#define N_TRY 3
#endif

#define GET_SSL_ERR() ERR_error_string(ERR_get_error(), NULL)

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

void print_ssl_session_info(std::shared_ptr<SSL> ssl,
                            SSL_SESSION *ssl_session) {
  absl::PrintF("ssl session info begin---\n");

  auto version = SSL_get_version(ssl.get());
  absl::PrintF("version: %s\n", version);

  auto reused = SSL_session_reused(ssl.get());
  absl::PrintF("reused: %d\n", reused);

  auto cipher = SSL_CIPHER_get_name(SSL_SESSION_get0_cipher(ssl_session));
  absl::PrintF("cipher: %s\n", cipher);

  auto client_random = [&]() {
    std::array<unsigned char, 1024> buffer;
    size_t len = SSL_get_client_random(ssl.get(), nullptr, 0);
    SSL_get_client_random(ssl.get(), buffer.data(), len);
    return std::string((const char *)buffer.data(), len);
  }();
  absl::PrintF("client random: %s\n", absl::BytesToHexString(client_random));

  auto server_random = [&]() {
    std::array<unsigned char, 1024> buffer;
    size_t len = SSL_get_server_random(ssl.get(), nullptr, 0);
    SSL_get_server_random(ssl.get(), buffer.data(), len);
    return std::string((const char *)buffer.data(), len);
  }();
  absl::PrintF("server random: %s\n", absl::BytesToHexString(server_random));

  auto session_id = [&]() {
    unsigned int sid_len = 0;
    auto sid = SSL_SESSION_get_id(ssl_session, &sid_len);
    return absl::string_view((const char *)sid, sid_len);
  }();
  absl::PrintF("session id: %s\n", absl::BytesToHexString(session_id));

  auto has_ticket = SSL_SESSION_has_ticket(ssl_session);
  absl::PrintF("has ticket: %d\n", has_ticket);

  if (has_ticket) {
    auto session_ticket = [&]() {
      const unsigned char *ticket;
      size_t len;
      SSL_SESSION_get0_ticket(ssl_session, &ticket, &len);
      return std::string((const char *)ticket, len);
    }();
    absl::PrintF("session ticket: %s\n",
                 absl::BytesToHexString(session_ticket));
  }

  auto master_key = [&]() {
    std::array<unsigned char, 1024> buffer;
    size_t len = SSL_SESSION_get_master_key(ssl_session, nullptr, 0);
    SSL_SESSION_get_master_key(ssl_session, buffer.data(), len);
    return std::string((const char *)buffer.data(), len);
  }();
  absl::PrintF("master key: %s\n", absl::BytesToHexString(master_key));

  absl::PrintF("---ssl session info end\n");
}

std::shared_ptr<SSL> create_ssl_conn(std::shared_ptr<SSL_CTX> ctx,
                                     std::shared_ptr<SSL_SESSION> *session_ptr,
                                     std::string sni,
                                     std::shared_ptr<int> tcp_conn) {
  assert(tcp_conn != nullptr);

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

  if (SSL_set_tlsext_host_name(ssl.get(), sni.c_str()) != 1) {
    absl::PrintF("unable to set tls sni: %s\n", sni);
    return nullptr;
  }

  SSL_set_fd(ssl.get(), *(tcp_conn.get()));

  // SSL_set_options(ssl.get(), SSL_OP_NO_TICKET);

  assert(session_ptr != nullptr);
  auto session = *session_ptr;
  if (session) {
    if (SSL_set_session(ssl.get(), session.get()) != 1) {
      absl::PrintF("unable to set reuse session. %s\n", GET_SSL_ERR());
      return nullptr;
    } else {
      absl::PrintF("reusing ssl session\n");
    }
  } else {
    absl::PrintF("no session usable\n");
  }

  if (SSL_connect(ssl.get()) != 1) {
    absl::PrintF("unable to start TLS negotiation with %s. %s\n", sni,
                 GET_SSL_ERR());
    return nullptr;
  }

  /* Grab session to store it */
  auto ssl_session = SSL_get1_session(ssl.get());
  if (ssl_session) {
    session_ptr->reset(ssl_session,
                       [](SSL_SESSION *ptr) { SSL_SESSION_free(ptr); });

    print_ssl_session_info(ssl, ssl_session);
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

int main(int argc, char *argv[]) {
  absl::SetProgramUsageMessage(
      "Request each ip:port in turn to check reuse of tls session");
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
    absl::PrintF("checking %s on %s\n", sni, AbslUnparseFlag(ip_ports));
  }

  SSL_load_error_strings();
  SSL_library_init();

  std::shared_ptr<SSL_CTX> ctx(SSL_CTX_new(TLS_client_method()), SSL_CTX_free);
  if (ctx == nullptr) {
    absl::PrintF("unable to init ssl context. %s\n", GET_SSL_ERR());
    exit(-1);
  }

  std::shared_ptr<SSL_SESSION> session;

  for (int i = 0; i < N_LOOP; ++i) {
    for (auto target : ip_ports) {
      auto tcp_conn_clock = base::util::SimpleClock("tcp_conn_cost");
      auto tcp = create_tcp_conn(target.ip, target.port);
      assert(tcp != nullptr);
      absl::PrintF("tcp conn cost %lldms\n", tcp_conn_clock.end());

      auto tls_conn_clock = base::util::SimpleClock("tls_conn_cost");
      auto ssl = create_ssl_conn(ctx, &session, sni, tcp);
      assert(ssl != nullptr);
      absl::PrintF("tls conn cost %lldms\n", tls_conn_clock.end());

      for (int j = 0; j < N_TRY; ++j) {
        auto http_clock = base::util::SimpleClock("http_cost");
        do_http_over_ssl(ssl, sni);
        absl::PrintF("http write read cost %lldms\n", http_clock.end());

        std::cout << std::endl;
      }
    }
  }

  return 0;
}
