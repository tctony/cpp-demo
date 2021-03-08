#include <iostream>

#include "asio.hpp"

using asio::ip::tcp;

int main(int argc, char const* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: client <host>" << std::endl;
    return 1;
  }

  try {
    asio::io_context ctx;

    tcp::resolver resolver(ctx);
    auto endpoints = resolver.resolve(argv[1], "daytime");

    tcp::socket socket(ctx);
    asio::connect(socket, endpoints);
    
    for (;;) {
      std::array<char, 128> buf;
      asio::error_code ec;

      auto len = socket.read_some(asio::buffer(buf), ec);
      if (ec == asio::error::eof) 
        break;
      else if (ec) {
        throw asio::system_error(ec);
      }

      std::cout.write(buf.data(), len);
    }
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
