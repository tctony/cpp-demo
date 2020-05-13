// clang-format off
#define ZLOG_TAG "client_async"
#include "base/zlog/zlog_to_console.h"
// clang-format on

#include <string>
#include <memory>
#include <chrono>

#include "grpcpp/grpcpp.h"
#include "demo/grpcdemo/proto/greeter.grpc.pb.h"

class GreeterClient {
 public:
  static GreeterClient &instance() {
    static GreeterClient instance_;
    return instance_;
  }

  void run() {
    std::string addr("0.0.0.0:50001");
    auto channel =
        grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
    stub_ = greeter::Greeter::NewStub(channel);

    running_ = true;

    stream_ = stub_->SayHello3(&context_);

    std::thread readThread([this] {
      zinfo_scope("read thread");

      greeter::HelloResponse response;
      while (stream_->Read(&response)) {
        zinfo("got response %_", response.message());
      }
    });

    std::thread writeThread([this] {
      zinfo_scope("write thread");

      greeter::HelloRequest request;
      request.set_name("tony");

      while (running_) {
        if (!stream_->Write(request)) {
          zerror("write error");
          break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    });

    if (readThread.joinable()) readThread.join();
    if (writeThread.joinable()) writeThread.join();
  }

  void stop() {
    if (!running_) return;
    running_ = false;
    {
      zinfo_scope("try canncel context");
      context_.TryCancel();
    }
  }

 private:
  std::atomic_bool running_{false};

  std::unique_ptr<greeter::Greeter::Stub> stub_;
  grpc::ClientContext context_;
  using ClientStream =
      grpc::ClientReaderWriter<greeter::HelloRequest, greeter::HelloResponse>;
  std::unique_ptr<ClientStream> stream_;
};

void signalHandler(int signal) {
  zinfo("handle signal: %_", signal);
  GreeterClient::instance().stop();
}

int main(int argc, char const *argv[]) {
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  GreeterClient::instance().run();
  return 0;
}
