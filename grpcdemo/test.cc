#include <iostream>

#include "gtest/gtest.h"
#include "service_impl.h"

using namespace grpc;
using namespace greeter;

TEST(Greeter, SayHello) {
  ServiceImpl service;
  ServerContext context;
  HelloRequest req;
  HelloResponse resp;
  req.mutable_config()->set_double_quote(true);
  req.set_name("tony");
  service.SayHello(&context, &req, &resp);
  std::cout << resp.message() << std::endl;
  // TODO: why Segmentation fault: 11?
}
