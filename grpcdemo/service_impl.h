#include <grpcpp/grpcpp.h>

#include "grpcdemo/proto/greeter.grpc.pb.h"
#include "grpcdemo/proto/greeter.pb.h"

namespace greeter {

class ServiceImpl final : public Greeter::Service {
 public:
  ::grpc::Status SayHello(::grpc::ServerContext* context,
                          const ::greeter::HelloRequest* request,
                          ::greeter::HelloResponse* response) override;

  ::grpc::Status SayHello1(
      ::grpc::ServerContext* context,
      ::grpc::ServerReader< ::greeter::HelloRequest>* reader,
      ::greeter::HelloResponse* response) override;

  ::grpc::Status SayHello2(
      ::grpc::ServerContext* context, const ::greeter::HelloRequest* request,
      ::grpc::ServerWriter< ::greeter::HelloResponse>* response) override;

  ::grpc::Status SayHello3(
      ::grpc::ServerContext* context,
      ::grpc::ServerReaderWriter< ::greeter::HelloResponse,
                                  ::greeter::HelloRequest>* stream) override;
};

}  // namespace greeter