#include <array>
#include <string>

#include <grpc/grpc.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "base/util/time_util.hpp"
#include "base/zlog/zlog.h"
#include "base/zlog/zlog_to_console.h"
#include "demo/grpcdemo/proto/greeter.grpc.pb.h"
#include "demo/grpcdemo/proto/greeter.pb.h"

using greeter::Greeter;
using greeter::HelloRequest;
using greeter::HelloResponse;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::CreateChannel;
using grpc::InsecureChannelCredentials;
using grpc::Status;

ZlogToConsoleSetup;

int main(int argc, char const *argv[]) {
  std::string addr("0.0.0.0:50001");
  auto channel = CreateChannel(addr, InsecureChannelCredentials());
  auto stub = Greeter::NewStub(channel);

  {
    // Sayhello
    ClientContext context;
    HelloRequest request;

    request.mutable_config()->set_double_quote(true);
    request.set_name("tony");
    HelloResponse response;
    auto cost = base::util::timeCostInMilliseconds([&] {
      auto status = stub->SayHello(&context, request, &response);
      if (!status.ok()) {
        zerror("rpc failed: %_ %_", status.error_code(),
               status.error_message());
      } else {
        zinfo("response message: %_", response.message());
      }
    });
    zinfo("cost %_ms", cost);
  }

  {
    // SayHello1
    ClientContext context;
    HelloResponse response;
    auto writer = stub->SayHello1(&context, &response);
    std::array<std::string, 3> names({
        "tony",
        "tang",
        "chang",
    });
    for (auto name : names) {
      HelloRequest req;
      req.set_name(name);
      if (!writer->Write(req)) {
        zinfo("write failed");
        break;
      }
    }
    writer->WritesDone();
    auto status = writer->Finish();
    if (!status.ok()) {
      zerror("rpc failed: %_ %_", status.error_code(), status.error_message());
    } else {
      zinfo("1 response message: %_", response.message());
    }
  }

  {
    // SayHello2
    ClientContext context;
    HelloRequest request;

    request.mutable_config()->set_double_quote(true);
    request.set_name("tony");
    auto reader = stub->SayHello2(&context, request);
    HelloResponse response;
    while (reader->Read(&response)) {
      zinfo("2 response message: %_", response.message());
    }
    auto status = reader->Finish();
    if (!status.ok()) {
      zerror("rpc failed: %_ %_", status.error_code(), status.error_message());
    }
  }

  {
    // SayHello3
    ClientContext context;
    std::shared_ptr<ClientReaderWriter<HelloRequest, HelloResponse>> stream(
        stub->SayHello3(&context));

    std::thread writer([stream] {
      std::array<std::string, 3> names({
          "tony",
          "tang",
          "chang",
      });
      for (auto name : names) {
        HelloRequest req;
        req.set_name(name);
        if (!stream->Write(req)) {
          zinfo("write failed");
          break;
        }
      }
      stream->WritesDone();
    });

    HelloResponse response;
    while (stream->Read(&response)) {
      zinfo("3 response message: %_", response.message());
    }

    writer.join();

    auto status = stream->Finish();
    if (!status.ok()) {
      zerror("rpc failed: %_ %_", status.error_code(), status.error_message());
    }
  }

  return 0;
}
