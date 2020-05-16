#define ZLOG_TAG "client_async"
#include "base/zlog/zlog_to_console.h"
// clang-format on

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "base/thread/thread_group.hpp"
#include "demo/grpcdemo/proto/greeter.grpc.pb.h"
#include "grpcpp/grpcpp.h"

using callback = std::function<void(int result)>;

class GreeterClient : std::enable_shared_from_this<GreeterClient> {
 public:
  GreeterClient() {}
  ~GreeterClient() {
    zinfo_function();
    stop();
  }

  void run() {
    if (running_) return;

    running_ = true;

    std::string addr("0.0.0.0:50001");
    auto channel =
        grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
    stub_ = greeter::Greeter::NewStub(channel);
    stream_ = stub_->AsyncSayHello3(&context_, &cq_, (void *)0);

    zinfo("start callback thread");
    thread_.createThread([this] {
      zinfo_scope("client callback thread");
      while (running_) {
        void *tag;
        bool ok;
        if (!cq_.Next(&tag, &ok)) {
          break;
        }
        zdebug("got next ")(tag, ok);
        if (!ok) {
          zinfo("session closed");
          break;
        }
        if (tag == nullptr && ok) {
          if (notify_.message().size() == 0) {
            zinfo("ready to read and write");
          } else {
            zinfo("got notify: %_", notify_.message());
          }
          readNextNotify();
          continue;
        }
        if (tag != nullptr) {  // request sent
          auto ctx = static_cast<RequestContext *>(tag);
          if (ok) {
            ctx->cb_(0);
          } else {
            // cancel or error?
            ctx->cb_(-1);
          }
          delete ctx;
        }
      }

      running_ = false;
    });
  }

  void readNextNotify() { stream_->Read(&notify_, (void *)0); }

  void sendRequest(const greeter::HelloRequest &req, callback cb) {
    if (!running_) {
      cb(-2);
      return;
    }
    zassert(stream_ != nullptr && "should call run first!");
    RequestContext *ctx = new RequestContext(std::move(cb));
    stream_->Write(req, (void *)ctx);
  }

  void stop() {
    zinfo_function();
    if (!running_) return;
    running_ = false;

    context_.TryCancel();
    cq_.Shutdown();
    thread_.join();
  }

 private:
  struct RequestContext {
    RequestContext(callback cb) : cb_(std::move(cb)) {}
    callback cb_;
  };

  std::atomic_bool running_{false};

  std::unique_ptr<greeter::Greeter::Stub> stub_;
  grpc::ClientContext context_;
  grpc::CompletionQueue cq_;
  using ClientStream = grpc::ClientAsyncReaderWriter<greeter::HelloRequest,
                                                     greeter::HelloResponse>;
  std::unique_ptr<ClientStream> stream_;
  greeter::HelloResponse notify_;

  base::ThreadGroup thread_;
};

GreeterClient *g_client = nullptr;
std::atomic_bool g_running = false;
void signalHandler(int signal) {
  zinfo("handle signal: %_", signal);
  g_running = false;
  if (g_client) {
    g_client->stop();
  }
}

int main(int argc, char const *argv[]) {
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  auto client = std::make_shared<GreeterClient>();
  g_client = client.get();

  client->run();
  g_running = true;

  for (int i = 0; i < 100 && g_running; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (!g_running) break;

    greeter::HelloRequest req;
    req.set_name("tony");
    client->sendRequest(req, [i](int result) {
      zinfo("send request %_ finished: %_", i, result);
    });
  }

  client.reset();

  return 0;
}
