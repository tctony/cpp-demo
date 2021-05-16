#include <iostream>

#include <ThreadPool.h>
#include <asio.hpp>

class Counter {
public:
  Counter(asio::io_context &ctx, int interval, int count)
      : strand_(asio::make_strand(ctx)),
        timer1_(ctx, asio::chrono::seconds(interval)),
        timer2_(ctx, asio::chrono::seconds(interval)), interval_(interval),
        count_(count) {
    assert(count > 0);

    timer1_.async_wait(
        asio::bind_executor(strand_, std::bind(&Counter::print1, this)));

    timer2_.async_wait(
        asio::bind_executor(strand_, std::bind(&Counter::print2, this)));
  }

  void print1() {
    if (!count_)
      return;

    std::cout << "timer1 " << count_ << std::endl;
    if (--count_) {
      timer1_.expires_at(timer1_.expiry() + asio::chrono::seconds(interval_));
      timer1_.async_wait(
          asio::bind_executor(strand_, std::bind(&Counter::print1, this)));
    }
  }

  void print2() {
    if (!count_)
      return;

    std::cout << "timer2 " << count_ << std::endl;
    if (--count_) {
      timer2_.expires_at(timer2_.expiry() + asio::chrono::seconds(interval_));
      timer2_.async_wait(
          asio::bind_executor(strand_, std::bind(&Counter::print2, this)));
    }
  }

private:
  asio::strand<asio::io_context::executor_type> strand_;
  asio::steady_timer timer1_;
  asio::steady_timer timer2_;
  int interval_;
  int count_;
};

int main(int argc, char const *argv[]) {
  asio::io_context ctx;
  auto _ = Counter(ctx, 1, 10);

  int n_threads = 2;
  auto pool = std::make_unique<ThreadPool>(n_threads);
  for (int i = 0; i < n_threads; ++i) {
    pool->enqueue([&] { ctx.run(); });
  }
  pool.reset();

  std::cout << "finished" << std::endl;

  return 0;
}
