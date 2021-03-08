#include <iostream>

#include <asio.hpp>

void print(const asio::error_code& ec, asio::steady_timer *timer, int *count) {
    std::cout << ++*count << std::endl;
    if (*count < 5) {
      timer->expires_at(timer->expiry() + asio::chrono::seconds(1));
      timer->async_wait(
        std::bind(print, std::placeholders::_1, timer, count));
    }
}

int main(int argc, char const *argv[])
{
  asio::io_context ctx;
  asio::steady_timer timer(ctx, asio::chrono::seconds(1));

  int count = 0;
  timer.async_wait(std::bind(print, std::placeholders::_1, &timer, &count));
  ctx.run();

  std::cout << "final count " << count << std::endl;

  return 0;
}
