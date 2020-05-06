#include <grpc/grpc.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <string>

#include "grpcdemo/proto/greeter.grpc.pb.h"
#include "grpcdemo/proto/greeter.pb.h"
#include "util/time_util.hpp"
#include "zlog/zlog.h"
#include "zlog/zlog_to_console.h"

using greeter::Greeter;
using greeter::HelloRequest;
using greeter::HelloResponse;
using grpc::ClientContext;
using grpc::CreateChannel;
using grpc::InsecureChannelCredentials;
using grpc::Status;

ZlogToConsoleSetup;

int main(int argc, char const *argv[]) {
  std::string addr("0.0.0.0:50001");
  auto channel = CreateChannel(addr, InsecureChannelCredentials());
  auto stub = Greeter::NewStub(channel);

  ClientContext context;
  HelloRequest request;
  request.mutable_config()->set_double_quote(true);
  request.set_name("tony");
  HelloResponse response;
  auto cost = base::util::timeCostInMilliseconds([&] {
    auto status = stub->SayHello(&context, request, &response);
    if (!status.ok()) {
      zerror("rpc failed: %_ %_", status.error_code(), status.error_message());
    } else {
      zinfo("response message: %_", response.message());
    }
  });
  zinfo("cost %_ms", cost);

  return 0;
}
