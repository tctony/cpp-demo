#include <functional>
#include <iostream>
#include <thread>

#include "absl/strings/str_cat.h"
#include "absl/synchronization/mutex.h"
#include "gtest/gtest.h"

class Thread {
 public:
  Thread(std::thread t) : thread_(std::move(t)) {}
  ~Thread() {
    if (thread_.joinable()) thread_.join();
  }

  Thread(Thread&& other) : thread_(std::move(other.thread_)) {}

 private:
  std::thread thread_;
};

TEST(sync, lock_unlock) {
  absl::Mutex lk;  // protects inventory
  int inventory = 0;
  bool stop = false;

  auto print_inventory = [&inventory](absl::string_view c) {
    static int last = 0;
    EXPECT_EQ(abs(last - inventory), 1);
    last = inventory;
    std::cout << absl::StrCat(c, " inventory: ", last, "\n");
  };

  std::vector<Thread> producers;
  for (int i = 0; i < 10; ++i) {
    producers.emplace_back(std::thread(
        [&](int i) {
          while (!stop) {
            auto condition = absl::Condition(
                +[](int* inv) { return *inv < 30; }, &inventory);
            if (true == lk.LockWhenWithTimeout(condition, absl::Seconds(1))) {
              ++inventory;
              print_inventory(absl::StrCat("p", i));
            }
            lk.Unlock();
          }
          std::cout << absl::StrCat("p", i, " stoped\n");
        },
        i));
  }

  std::vector<Thread> consumers;
  for (int i = 0; i < 3; ++i) {
    consumers.emplace_back(std::thread(
        [&](int i) {
          while (!stop) {
            auto condition = absl::Condition(
                +[](int* inv) { return *inv > 0; }, &inventory);
            if (true == lk.LockWhenWithTimeout(condition, absl::Seconds(1))) {
              --inventory;
              print_inventory(absl::StrCat("c", i));
            }
            lk.Unlock();
          }
          std::cout << absl::StrCat("c", i, " stoped\n");
        },
        i));
  }

  sleep(3);
  stop = true;
}