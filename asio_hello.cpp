#include <thread>

#include "asio.hpp"

int main(int argc, char const *argv[]) {
  asio::io_context io_context;
  auto guard = asio::make_work_guard(io_context);

  std::thread t([](asio::io_context *ctx) { ctx->run(); }, &io_context);

  sleep(3);
  guard.reset();
  io_context.stop();
  t.join();

  return 0;
}
