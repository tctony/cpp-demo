#include <ctime>
#include <iostream>

#include "asio.hpp"

using asio::ip::tcp;

std::string make_daytime_string() {
  using namespace std;
  time_t now = time(0);
  return ctime(&now);
}

class connection : public std::enable_shared_from_this<connection> {
 public:
  connection(asio::io_context& ctx) : socket_(ctx) {
    // do noting
  }

  tcp::socket& socket() { return socket_; }

  void handle() {
    message_ = make_daytime_string();
    auto completion = std::bind(&connection::onWrite, shared_from_this(),
                                std::placeholders::_1, std::placeholders::_2);
    asio::async_write(socket_, asio::buffer(message_), std::move(completion));
  }

  void onWrite(const asio::error_code& ec, size_t nwrite) {
    std::cout << "finish one job" << std::endl;
  }

 private:
  tcp::socket socket_;
  std::string message_;
};

class server {
 public:
  server(asio::io_context& ctx)
      : ctx_(ctx), acceptor_(ctx, tcp::endpoint(tcp::v4(), 13)) {
    doAccept();
  }

 private:
  void doAccept() {
    auto conn = std::make_shared<connection>(ctx_);
    auto completion =
        std::bind(&server::handleAccept, this, conn, std::placeholders::_1);
    acceptor_.async_accept(conn->socket(), std::move(completion));
  }

  void handleAccept(std::shared_ptr<connection> conn,
                    const asio::error_code& ec) {
    if (!ec) {
      conn->handle();
    }
    doAccept();
  }

  asio::io_context& ctx_;
  tcp::acceptor acceptor_;
};

int main(int argc, char const* argv[]) {
  try {
    asio::io_context ctx;
    server server(ctx);
    ctx.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
  }

  return 0;
}
