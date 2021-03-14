#include <iostream>

#include "asio.hpp"
#include "absl/strings/str_cat.h"
#include "base/util/disablecopy.h"

enum CustomErrorCode {
  noError = 0,
  clientQuit = 1,
  serverQuit = 2,
};

class CustomErrorCategory : public std::error_category {
public:
  constexpr CustomErrorCategory() {};
  CLASS_DISABLE_COPY(CustomErrorCategory);

  virtual const char* name() const noexcept override {
    return "CustomErrorCategory";
  }
  virtual std::string message(int value) const override {
    if (value == 0) {
      return "no error";
    } else if (value == clientQuit) {
      return "client quit";
    } else if (value == serverQuit) {
      return "server quit";
    } else {
      return absl::StrCat("unspecified ", this->name(), " error: ", value);
    }
  }
};

namespace std {
template<> struct is_error_condition_enum<CustomErrorCode> : true_type {};
} // namespace std

static CustomErrorCategory customErrorCategory;
inline std::error_condition make_error_condition(CustomErrorCode ec) {
  return std::error_condition(static_cast<int>(ec), customErrorCategory);
}

using CustomError = std::error_condition;

using asio::ip::tcp;

void talkToServer(tcp::socket& socket, CustomError& error) {
  std::cout << "client: ";
  std::string line;
  std::getline(std::cin, line);
  if (!line.size()) {
    error = clientQuit;
    return;
  }
  asio::error_code ec;
  socket.write_some(asio::buffer(line), ec);
  if (ec) {
    throw asio::system_error(ec);
  }
}

inline std::string readFromServer(tcp::socket& socket, CustomError& error) {
  std::array<char, 128> buf;
  asio::error_code ec;

  auto len = socket.read_some(asio::buffer(buf), ec);
  if (ec == asio::error::eof) {
    error = serverQuit;
    return "";
  } else if (ec) {
    throw asio::system_error(ec);
  } else {
    return std::string(buf.data(), len);
  }
}

int main(int argc, char const* argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: client <host> <port>" << std::endl;
    return 1;
  }

  try {
    asio::io_context ctx;

    tcp::resolver resolver(ctx);
    auto endpoints = resolver.resolve(argv[1], argv[2]);

    tcp::socket socket(ctx);
    asio::connect(socket, endpoints);

    CustomError error;
    auto server_hello = readFromServer(socket, error);
    assert(!error);
    std::cout << "connected: " << server_hello;

    for (;;) {
      talkToServer(socket, error);
      if (clientQuit == error) {
        std::cout << "client quit" << std::endl;
        break;
      } else if (error) {
        std::cout << "write errror: " << error.message() << std::endl;
        assert(0);
      }

      auto msg = readFromServer(socket, error);
      if (serverQuit == error) {
        std::cout << "server quit" << std::endl;
        break;
      } else if (error) {
        std::cout << "read errror: " << error.message() << std::endl;
        assert(0);
      }
      std::cout << "server: " << msg;
    }
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
