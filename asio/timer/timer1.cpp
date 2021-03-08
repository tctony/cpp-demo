#include <iostream>

#include <asio.hpp>

int main(int argc, char const *argv[])
{
  asio::io_context io_ctx;
  asio::steady_timer timer(io_ctx, asio::chrono::seconds(3));
  timer.wait();
  std::cout << "timer expired" << std::endl;

  return 0;
}
