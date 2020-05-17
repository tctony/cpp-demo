// clang-format off
#define ZLOG_TAG "server_async"
#include "base/zlog/zlog_to_console.h"
// clang-format on

#include <string>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <tuple>
#include <thread>
#include <chrono>
#include <mutex>
#include <deque>

#include "service_impl.h"
#include "base/util/disablecopy.h"
#include "base/thread/thread_group.hpp"

#define SESSION_EVENT_BIT_LENGTH 2
#define SESSION_EVENT_MASK ((1 << SESSION_EVENT_BIT_LENGTH) - 1)

enum SessionEvent {
  SessionEventConnected = 0,
  SessionEventReadDone = 1,
  SessionEventWriteDone = 2,
  SessionEventFinished = 3,
};

void* tagify(uint64_t session_id, SessionEvent event) {
  return reinterpret_cast<void*>(session_id << SESSION_EVENT_BIT_LENGTH |
                                 (uint64_t)event);
}

std::tuple<uint64_t, SessionEvent> detagify(void* tag) {
  uint64_t value = reinterpret_cast<uint64_t>(tag);
  return std::make_tuple(value >> SESSION_EVENT_BIT_LENGTH,
                         (SessionEvent)(value & SESSION_EVENT_MASK));
}

enum SessionStatus {
  SessionStatusWaitToConnect = 0,
  SessionStatusReadyToWrite,
  SessionStatusWaitWriteDone,
  //  SessionStatusFinished,
};

class GreeterSession;
class GreeterServer {
 public:
  static GreeterServer* Instance() {
    static GreeterServer instance_;
    return &instance_;
  }
  CLASS_DISABLE_COPY_MOVE(GreeterServer);

 private:
  explicit GreeterServer() {}
  ~GreeterServer() {}

 public:
  void Start(std::string addr);
  void Stop();

 private:
  std::shared_ptr<base::ThreadGroup> StartAccepterThread();
  std::shared_ptr<base::ThreadGroup> StartWorkerThread();
  std::shared_ptr<base::ThreadGroup> StartBroadcastThread();

 private:  // session
  friend class GreeterSession;

  std::shared_ptr<GreeterSession> AddNewSession();
  std::shared_ptr<GreeterSession> GetSession(uint64_t session_id);
  void RemoveSession(uint64_t session_id);

 private:
  std::atomic_bool running_{false};

  std::unique_ptr<grpc::Server> server_{};
  greeter::Greeter::AsyncService async_service_{};
  std::unique_ptr<grpc::ServerCompletionQueue> cq_readwrite_;
  std::unique_ptr<grpc::ServerCompletionQueue> cq_conn_;

  std::shared_mutex mutex_sessions_;
  uint64_t session_id_allocator_{0};
  std::unordered_map<uint64_t /*session_id*/, std::shared_ptr<GreeterSession>>
      sessions_{};
};

class GreeterSession {
 public:
  GreeterSession(GreeterServer& server, uint64_t session_id)
      : server_(server), session_id_(session_id) {
    server_.async_service_.RequestSayHello3(
        &context_, &stream_, server_.cq_readwrite_.get(),
        server_.cq_conn_.get(), tagify(session_id_, SessionEventConnected));
  }

 public:
  void ProcessEvent(SessionEvent event) {
    std::lock_guard<std::mutex> lock_guard{mutex_};
    zinfo()(session_id_, status_, event);
    switch (event) {
      case SessionEventConnected: {
        stream_.Read(&request_, tagify(session_id_, SessionEventReadDone));
        status_ = SessionStatus::SessionStatusReadyToWrite;
        return;
      }
      case SessionEventReadDone: {
        zinfo("received request %_, ",
              request_.ShortDebugString())(session_id_);
        auto response = std::make_shared<greeter::HelloResponse>();
        response->set_message("hello " + request_.name());
        SendResponse(response);
        stream_.Read(&request_, tagify(session_id_, SessionEventReadDone));
        return;
      }
      case SessionEventWriteDone: {
        if (!message_queue_.empty()) {
          status_ = SessionStatusWaitWriteDone;
          stream_.Write(*message_queue_.front(),
                        tagify(session_id_, SessionEventWriteDone));
          message_queue_.pop_front();
        } else {
          status_ = SessionStatusReadyToWrite;
        }
        return;
      }
      case SessionEventFinished: {
        // do nothing
        return;
      }
      default: {
        zinfo("unexpected event %_ session_id %_", event, session_id_);
        assert(0);
        return;
      }
    }
  }

  void SendResponse(std::shared_ptr<greeter::HelloResponse> response) {
    if (status_ != SessionStatusReadyToWrite &&
        status_ != SessionStatusWaitWriteDone) {
      return;
    }
    if (status_ == SessionStatusReadyToWrite) {
      status_ = SessionStatusWaitWriteDone;
      stream_.Write(*response, tagify(session_id_, SessionEventWriteDone));
    } else {
      message_queue_.emplace_back(response);
    }
  }

 private:
  friend class GreeterServer;
  GreeterServer& server_;

  uint64_t session_id_;

  grpc::ServerContext context_{};
  using SessionStream = grpc::ServerAsyncReaderWriter<greeter::HelloResponse,
                                                      greeter::HelloRequest>;
  SessionStream stream_{&context_};

  std::mutex mutex_{};
  SessionStatus status_{SessionStatusWaitToConnect};

  greeter::HelloRequest request_{};
  std::deque<std::shared_ptr<greeter::HelloResponse>> message_queue_;
};

void GreeterServer::Start(std::string addr) {
  if (running_) return;
  running_ = true;

  grpc::ServerBuilder builder;
  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  builder.RegisterService(&async_service_);
  cq_conn_ = builder.AddCompletionQueue();
  cq_readwrite_ = builder.AddCompletionQueue();

  server_ = builder.BuildAndStart();

  zinfo("server listening on %_", addr);

  auto threads = std::make_tuple(StartAccepterThread(), StartWorkerThread(),
                                 StartBroadcastThread());
}

std::shared_ptr<base::ThreadGroup> GreeterServer::StartAccepterThread() {
  auto thread_accepter = std::make_shared<base::ThreadGroup>();
  thread_accepter->createThread([this] {
    while (running_) {
      zinfo("thread accepter started");
      AddNewSession();
      void* tag;
      bool ok;
      while (cq_conn_->Next(&tag, &ok)) {
        auto [session_id, event] = detagify(tag);
        zinfo("thread_accepter got: ")(ok, session_id, event);

        auto session = GetSession(session_id);

        if (session == nullptr) {
          zwarn("session not found %_", session_id);
          continue;
        }

        if (!ok) {
          zinfo("session %_ close on waiting", session_id);
          RemoveSession(session_id);
          continue;
        }

        {  // new connection
          zassert(event == SessionEventConnected);
          zinfo("session %_ accepted", session_id);
          session->ProcessEvent(event);

          AddNewSession();
        }
      }
      zinfo("thread accepter finished");
    }
  });
  return thread_accepter;
}

std::shared_ptr<base::ThreadGroup> GreeterServer::StartWorkerThread() {
  auto thread_worker = std::make_shared<base::ThreadGroup>();
  thread_worker->createThread([this] {
    zinfo("thread worker started");
    while (running_) {
      void* tag;
      bool ok;
      while (cq_readwrite_->Next(&tag, &ok)) {
        auto [session_id, event] = detagify(tag);
        zinfo("thread worker got: ")(ok, session_id, event);

        auto session = GetSession(session_id);

        if (session == nullptr) {
          zwarn("session not found %_", session_id);
          continue;
        }

        if (!ok) {
          zinfo("session %_ close on working", session_id);
          RemoveSession(session_id);
          continue;
        }

        {  // read done or write done
          zassert(event == SessionEventReadDone ||
                  event == SessionEventWriteDone);
          zinfo("session %_ %_ done", session_id,
                (event == SessionEventReadDone ? "read" : "write"));
          session->ProcessEvent(event);
        }
      }
    }
    zinfo("thread worker finished");
  });
  return thread_worker;
}

std::shared_ptr<base::ThreadGroup> GreeterServer::StartBroadcastThread() {
  auto thread_broadcast = std::make_shared<base::ThreadGroup>();
  thread_broadcast->createThread([this] {
    zinfo("thread broadcast started");
    auto message = std::make_shared<greeter::HelloResponse>();
    message->set_message("this is a broadcast message");
    while (running_) {
      std::shared_lock<std::shared_mutex> shared_lock_guard{mutex_sessions_};
      if (!running_) break;
      for (const auto& itr : sessions_) {
        std::lock_guard<std::mutex> lock_guard_inner{itr.second->mutex_};
        itr.second->SendResponse(message);
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    zinfo("thread broadcast finished");
  });
  return thread_broadcast;
}

void GreeterServer::Stop() {
  if (!running_) return;
  running_ = false;

  {
    zinfo_scope("try cancel sessions");
    std::lock_guard<std::shared_mutex> lock_guard{mutex_sessions_};

    for (const auto& itr : sessions_) {
      std::lock_guard<std::mutex> lock_guard_inner{itr.second->mutex_};
      if (itr.second->status_ != SessionStatusWaitToConnect) {
        zinfo("try cancel session %_", itr.second->session_id_);
        itr.second->context_.TryCancel();
      }
    }
  }
  {
    zinfo_scope("shutdown server");
    server_->Shutdown();
  }
  {
    zinfo_scope("shutdown cq_conn_");
    cq_conn_->Shutdown();
  }
  {
    zinfo_scope("shutdown cq_readwrite");
    cq_readwrite_->Shutdown();
  }
}

std::shared_ptr<GreeterSession> GreeterServer::AddNewSession() {
  std::lock_guard<std::shared_mutex> lock_guard{mutex_sessions_};
  auto new_session_id = session_id_allocator_++;
  zinfo("spawn session %_ and wait for connect", new_session_id);
  auto new_session = std::make_shared<GreeterSession>(*this, new_session_id);
  sessions_[new_session_id] = new_session;
  return new_session;
}

std::shared_ptr<GreeterSession> GreeterServer::GetSession(uint64_t session_id) {
  std::shared_lock<std::shared_mutex> shared_lock_guard{mutex_sessions_};
  auto itr = sessions_.find(session_id);
  if (itr != sessions_.end()) {
    return itr->second;
  }
  return nullptr;
}

void GreeterServer::RemoveSession(uint64_t session_id) {
  std::lock_guard<std::shared_mutex> lock_guard{mutex_sessions_};
  sessions_.erase(session_id);
}

void signal_handler(int signal) {
  zinfo("handle signal %_", signal);

  GreeterServer::Instance()->Stop();
}

int main(int argc, char const* argv[]) {
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  GreeterServer::Instance()->Start("127.0.0.1:50001");

  return 0;
}
