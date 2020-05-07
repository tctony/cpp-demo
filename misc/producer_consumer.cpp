#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

using namespace std;

int main(int argc, char *argv[]) {
  queue<int> buffer;
  mutex mtx;
  condition_variable cv;
  bool notified = false;

  auto producer = [&]() {
    for (int i = 0;; i++) {
      this_thread::sleep_for(chrono::milliseconds(900));
      unique_lock<mutex> lock(mtx);
      cout << "producing " << i << endl;
      buffer.push(i);
      notified = true;
      cv.notify_all();
    }
  };

  auto consumer = [&]() {
    while (true) {
      unique_lock<mutex> lock(mtx);
      while (!notified) {
        cv.wait(lock);
      }
      lock.unlock();
      this_thread::sleep_for(chrono::milliseconds(1000));
      lock.lock();
      while (!buffer.empty()) {
        cout << "consuming " << buffer.front() << endl;
        buffer.pop();
      }
      notified = false;
    }
  };

  thread p(producer);
  thread cs[2];
  for (int i = 0; i < 2; ++i) {
    cs[i] = thread(consumer);
  }
  p.join();
  for (int i = 0; i < 2; ++i) {
    cs[i].join();
  }

  return 0;
}
