#include <ctime>
#include <iostream>

#include "asio.hpp"

using asio::ip::tcp;

std::string make_daytime_string() {
  using namespace std;
  time_t now = time(0);
  return ctime(&now);
}

int main(int argc, char const* argv[]) {
  try {
    asio::io_context ctx;

    tcp::acceptor acceptor(ctx, tcp::endpoint(tcp::v4(), 13));

    for (;;) {
      tcp::socket socket(ctx);
      acceptor.accept(socket);

      std::string msg = make_daytime_string();

      asio::error_code ec;
      asio::write(socket, asio::buffer(msg), ec);
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
  }

  return 0;
}
