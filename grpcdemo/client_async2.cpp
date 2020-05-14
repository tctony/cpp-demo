#define ZLOG_TAG "client_async"
#include "base/zlog/zlog_to_console.h"
// clang-format on

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "base/thread/thread_group.hpp"
#include "base/thread/worker.hpp"
#include "demo/grpcdemo/proto/greeter.grpc.pb.h"
#include "grpcpp/grpcpp.h"

using callback = std::function<void()>;

class GreeterClient : std::enable_shared_from_this<GreeterClient> {
 public:
  GreeterClient(std::shared_ptr<base::Worker> worker) : worker_(worker) {
    zinfo_function();
    // init
  }
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
          zinfo("ready to read and write");
          readNextNotify();
          continue;
        }
        if (tag != nullptr) {  // request sent
          auto ctx = static_cast<RequestContext *>(tag);
          ctx->cb_();
          delete ctx;
        } else {  // notify
          readNextNotify();
          zinfo("got notify: %_", notify_.message());
        }
      }
    });
  }

  void readNextNotify() {
    // worker_->context()->post([client = shared_from_this()] {
    stream_->Read(&notify_, (void *)0);
    // });
  }

  void sendRequest(const greeter::HelloRequest &req, callback cb) {
    zassert(stream_ != nullptr && "should call run first!");
    // TODO post to worker
    RequestContext *ctx = new RequestContext(std::move(cb));
    stream_->Write(req, (void *)ctx);
  }

  void stop() {
    zinfo_function();
    if (!running_) return;

    context_.TryCancel();
    cq_.Shutdown();
    running_ = false;
    thread_.join();
  }

 private:
  struct RequestContext {
    RequestContext(callback cb) : cb_(std::move(cb)) {}
    callback cb_;
  };

 private:
  std::shared_ptr<base::Worker> worker_;

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
  if (g_client) {
    g_client->stop();
  }
  g_running = false;
}

int main(int argc, char const *argv[]) {
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  auto worker = std::make_shared<base::Worker>();
  auto client = std::make_shared<GreeterClient>(worker);
  g_client = client.get();

  client->run();
  g_running = true;

  for (int i = 0; i < 100 && g_running; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    greeter::HelloRequest req;
    req.set_name("tony");
    client->sendRequest(req, [i] { zinfo("send request %_ succeed", i); });
  }

  client.reset();
  worker.reset();

  return 0;
}
