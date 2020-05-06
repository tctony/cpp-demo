#include "service_impl.h"

#include <sstream>

Status ServiceImpl::SayHello(ServerContext* context,
                             const HelloRequest* request,
                             HelloResponse* response) {
  char quote = request->config().double_quote() ? '"' : '\'';
  std::stringstream ss;
  ss << "hello " << quote << request->name() << quote;
  response->set_message(ss.str());
  return Status::OK;
}