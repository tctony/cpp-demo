#include <grpc/grpc.h>
#include <grpcpp/server_context.h>

#include "grpcdemo/proto/greeter.grpc.pb.h"
#include "grpcdemo/proto/greeter.pb.h"

using greeter::Greeter;
using greeter::HelloRequest;
using greeter::HelloResponse;
using grpc::ServerContext;
using grpc::Status;

class ServiceImpl final : public Greeter::Service {
 public:
  Status SayHello(ServerContext* context, const HelloRequest* request,
                  HelloResponse* response);
};