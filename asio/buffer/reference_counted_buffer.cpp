#include <ctime>
#include <iostream>
#include <vector>

#include "absl/strings/str_cat.h"
#include "asio.hpp"

using asio::ip::tcp;

class handler_memory {
 public:
  handler_memory() : in_use_(false) {}

  handler_memory(const handler_memory&) = delete;
  handler_memory& operator=(const handler_memory&) = delete;

  void* allocate(size_t size) {
    if (!in_use_ && size < sizeof(storage_)) {
      std::cout << "use memory for size " << size << std::endl;
      in_use_ = true;
      return &storage_;
    } else {
      std::cout << "new memory for size " << size << std::endl;
      return ::operator new(size);
    }
  }

  void deallocate(void* pointer) {
    if (pointer == &storage_) {
      in_use_ = false;
      std::cout << "deuse" << std::endl;
    } else {
      std::cout << "delete" << std::endl;
      return ::operator delete(pointer);
    }
  }

 private:
  typename std::aligned_storage<1024>::type storage_;
  bool in_use_;
};

template <typename T>
class handler_allocator {
 public:
  using value_type = T;

  explicit handler_allocator(handler_memory& memory) : memory_(memory) {}

  // 用在什么情况下？
  template <typename U>
  handler_allocator(const handler_allocator<U>& other)
      : memory_(other.memory_) {}

  bool operator==(const handler_allocator& other) const noexcept {
    return &memory_ == &other.memory_;
  }

  bool operator!=(const handler_allocator& other) const noexcept {
    return &memory_ != &other.memory_;
  }

  T* allocate(std::size_t n) const {
    return static_cast<T*>(memory_.allocate(sizeof(T) * n));
  }

  void deallocate(T* p, std::size_t /*n*/) const { memory_.deallocate(p); }

 private:
  template <typename>
  friend class handler_allocator;

  handler_memory& memory_;
};

template <typename Handler>
class custom_alloc_handler {
 public:
  using allocator_type = handler_allocator<Handler>;

  custom_alloc_handler(handler_memory& memory, Handler handler)
      : memory_(memory), handler_(handler) {}

  allocator_type get_allocator() const noexcept {
    return allocator_type(memory_);
  }

  template <typename... Args>
  void operator()(Args&&... args) {
    handler_(std::forward<Args>(args)...);
  }

 private:
  handler_memory& memory_;
  Handler handler_;
};

template <typename Handler>
inline custom_alloc_handler<Handler> make_custom_alloc_handler(
    handler_memory& m, Handler h) {
  return custom_alloc_handler<Handler>(m, h);
}

std::string make_daytime_string() {
  using namespace std;
  time_t now = time(0);
  return ctime(&now);
}

class shared_const_buffer {
 public:
  shared_const_buffer(const std::string& str)
      : data_(new std::vector<char>(str.begin(), str.end())),
        buffer_(asio::buffer(*data_)) {}

  using value_type = asio::const_buffer;
  using const_iterator = const asio::const_buffer*;
  const asio::const_buffer* begin() const { return &buffer_; }
  const asio::const_buffer* end() const { return &buffer_ + 1; }

 private:
  std::shared_ptr<std::vector<char>> data_;
  asio::const_buffer buffer_;
};

class connection : public std::enable_shared_from_this<connection> {
 public:
  connection(tcp::socket socket) : socket_(std::move(socket)) {
    // do noting
  }

  void start() { doWrite(0); }

 private:
  void doWrite(std::size_t length) {
    auto buffer = shared_const_buffer(
        absl::StrCat("got ", absl::string_view(data_.data(), length), " on ",
                     make_daytime_string()));

    auto self(shared_from_this());
    asio::async_write(
        socket_, buffer,
        make_custom_alloc_handler(
            memory_, [this, self](std::error_code ec, std::size_t /*length*/) {
              if (!ec) {
                doRead();
              }
            }));
  }

  void doRead() {
    auto self(shared_from_this());
    socket_.async_read_some(
        asio::buffer(data_),
        make_custom_alloc_handler(
            memory_, [this, self](std::error_code ec, std::size_t length) {
              if (!ec) {
                this->doWrite(length);
              }
            }));
  }

  handler_memory memory_;
  tcp::socket socket_;
  std::array<char, 1024> data_;
};

class server {
 public:
  server(asio::io_context& ctx) : acceptor_(ctx, tcp::endpoint(tcp::v4(), 13)) {
    doAccept();
  }

 private:
  void doAccept() {
    acceptor_.async_accept([this](std::error_code ec, tcp::socket socket) {
      if (!ec) {
        std::make_shared<connection>(std::move(socket))->start();
      }

      doAccept();
    });
  }

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
