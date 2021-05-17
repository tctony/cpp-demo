#include <iostream>
#include <memory>
#include <typeinfo>

#include <asio.hpp>

template <typename T> struct Service;
template <typename T>
using ServiceBase = asio::detail::execution_context_service_base<Service<T>>;

struct ImplBase {
  virtual ~ImplBase() {}
  virtual void operator()() {
    std::cout << "calling " << typeid(*this).name() << "()\n";
  }
};
struct ImplA: ImplBase {};
struct ImplB: ImplBase {};
struct ImplC: ImplBase {};

template <typename T> struct Service : ServiceBase<T> {
  Service(asio::io_context &ctx) : ServiceBase<T>(ctx) {
    std::cout << "create service " << typeid(*this).name() << "\n";
  }
  ~Service() {
    std::cout << "destroy service " << typeid(*this).name() << "\n";
  }

  void shutdown() override {
    // do nothing
  }

  T value{};
};

int main(int argc, const char *argv[]) {
  auto shared_ptr = std::make_shared<ImplC>();
  (*shared_ptr)();

  {
    asio::io_context context;
    std::cout << "context created\n";

    {
      asio::use_service<Service<ImplA>>(context).value();
      asio::use_service<Service<ImplA>>(context).value();
    }

    {
      auto ptr = new Service<ImplB>(context);
      ptr->value();
      asio::add_service(context, ptr);
      asio::use_service<std::remove_reference<decltype(*ptr)>::type>(context).value();
    }

    {
      //new Service<std::shared_ptr<ImplC>>(context);
      // asio::make_service<Service<std::shared_ptr<ImplC>>>(context);
      auto &service = asio::use_service<Service<std::shared_ptr<ImplC>>>(context);
      assert(service.value == nullptr);
      service.value = shared_ptr;
      (*service.value)();
    }

    std::cout << "context released\n";
  }
  
  assert(shared_ptr != nullptr);

  std::cout << "before main return\n";

  return 0;
}