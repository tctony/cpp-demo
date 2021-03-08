#include <iostream>

#include <asio.hpp>

int main(int argc, char const *argv[])
{
  asio::io_context ctx;
  asio::steady_timer timer(ctx, asio::chrono::seconds(3));
  timer.async_wait([](const asio::error_code&){
    std::cout << "timer expired" << std::endl;
  });
  ctx.run();

  return 0;
}
