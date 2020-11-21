#include "service_impl.h"

#include <sstream>

#include "base/zlog/zlog.h"

using namespace grpc;

namespace greeter {

static inline std::string quotedName(const std::string& name,
                                     const common::Config& config) {
  char quote = config.double_quote() ? '"' : '\'';
  std::stringstream ss;
  ss << quote << name << quote;
  return ss.str();
}

Status ServiceImpl::SayHello(ServerContext* context,
                             const HelloRequest* request,
                             HelloResponse* response) {
  zinfo_function();

  std::stringstream ss;
  ss << "hello " << quotedName(request->name(), request->config());
  zinfo("%_", ss.str());
  response->set_message(ss.str());

  return Status::OK;
}

Status ServiceImpl::SayHello1(ServerContext* context,
                              ServerReader<HelloRequest>* reader,
                              HelloResponse* response) {
  zinfo_function();

  std::stringstream ss;
  ss << "hello";
  HelloRequest req;
  while (reader->Read(&req)) {
    ss << ' ' << quotedName(req.name(), req.config());
  }
  zinfo("%_", ss.str());
  response->set_message(ss.str());

  return Status::OK;
}

Status ServiceImpl::SayHello2(ServerContext* context,
                              const HelloRequest* request,
                              ServerWriter<HelloResponse>* writer) {
  zinfo_function();

  for (int i = 0; i < 3; ++i) {
    std::stringstream ss;
    ss << "hello" << quotedName(request->name(), request->config());
    zinfo("%_", ss.str());
    HelloResponse response;
    response.set_message(ss.str());
    if (!writer->Write(response)) {
      zinfo("write error");
      break;
    }
  }

  return Status::OK;
}

Status ServiceImpl::SayHello3(
    ServerContext* context,
    ServerReaderWriter<HelloResponse, HelloRequest>* stream) {
  zinfo_function();

  HelloRequest req;
  while (stream->Read(&req)) {
    std::stringstream ss;
    ss << "hello";
    ss << ' ' << quotedName(req.name(), req.config());
    zinfo("%_", ss.str());
    HelloResponse response;
    response.set_message(ss.str());
    stream->Write(response);
  }

  return Status::OK;
}

}  // namespace greeter
