#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server_builder.h>

#include <string>

#include "base/zlog/zlog_to_console.h"
#include "service_impl.h"

using greeter::ServiceImpl;
using grpc::Server;
using grpc::ServerBuilder;

ZlogToConsoleSetup;

void RunServer() {
  std::string addr("0.0.0.0:50001");
  ServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  auto server = builder.BuildAndStart();
  zinfo("Server listening on %_", addr);
  server->Wait();
};

int main(int argc, char const* argv[]) {
  RunServer();

  return 0;
}
