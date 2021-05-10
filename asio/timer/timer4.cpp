#include <asio.hpp>
#include <iostream>

class Counter {
 public:
  Counter(asio::io_context& ctx, int interval, int count)
      : timer_(ctx, asio::chrono::seconds(interval)),
        interval_(interval),
        count_(count) {
    assert(count > 0);
    timer_.async_wait(std::bind(&Counter::print, this));
  }

  void print() {
    std::cout << count_ << std::endl;
    if (--count_) {
      timer_.expires_at(timer_.expiry() + asio::chrono::seconds(interval_));
      timer_.async_wait(std::bind(&Counter::print, this));
    }
  }

 private:
  asio::steady_timer timer_;
  int interval_;
  int count_;
};

int main(int argc, char const* argv[]) {
  asio::io_context ctx;
  auto _ = Counter(ctx, 1, 3);
  ctx.run();
  std::cout << "finished" << std::endl;
  return 0;
}
