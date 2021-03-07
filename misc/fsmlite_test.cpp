#include <iostream>
#include <vector>

#ifndef external_fsmlite
#include "3rd/fsmlite/fsm.h"
#else
#include "fsm.h"
#endif
#include "absl/strings/str_split.h"
#include "base/http/request_formatter.hpp"
#include "base/util/string_util.hpp"

using namespace base::http;
using namespace base::util;

class HttpRequestParser_fsm : public fsmlite::fsm<HttpRequestParser_fsm> {
  friend class fsm;

 public:
  enum ParseState {
    Error = -1,
    Start = 0,
    Method,
    Uri,
    Http,
    Major,
    Minor,
    Expecting_newline_1,
    Header_line_start,
    Header_line,
    Expecting_newline_2,
    Expecting_newline_3,
    Expecting_body,
    Finish,
  };

  HttpRequestParser_fsm() : fsmlite::fsm<HttpRequestParser_fsm>(Start) {}

 private:
  Request request_;
  std::string temp_;

  std::string* buffer_{&request_.method};

 private:
  // checkers
  bool isAlpha(const char& c) const { return isalpha(c); }
  bool isDigit(const char& c) const { return isdigit(c); }
  bool isSpace(const char& c) const { return ' ' == c; }
  bool isCarryReturn(const char& c) const { return '\r' == c; }
  bool isNewLine(const char& c) const { return '\n' == c; }
  bool isSlash(const char& c) const { return '/' == c; }

  // actions
  void noAction(const char& c) { ; }
  void errorAction(const char& c) { ; }
  void appendToBuffer(const char& c) {
    if (nullptr != buffer_) buffer_->append(1, c);
  }

  bool isValidMethod(const char& c) const {
    return (' ' == c) &&
           ("GET" == request_.method || "POST" == request_.method);
  }
  void onMethodFinish(const char& c) { buffer_ = &(request_.uri); }

  bool isValidUriChar(const char& c) const {
    return isalpha(c) || isnumber(c) || ispunct(c);
  }
  bool isValidUri(const char& c) const {
    // check uri here
    return (' ' == c);
  }
  void onUriFinish(const char& c) {
    temp_.clear();
    buffer_ = &(temp_);
  }

  bool isValidHttp(const char& c) const {
    return ('/' == c && ("http" == toLowerCase(temp_)));
  }
  void onHttpFinish(const char& c) {
    temp_.clear();
    buffer_ = &(temp_);
  }

  bool isValidMajor(const char& c) const {
    if ('.' != c) return false;

    auto req = const_cast<decltype(request_)&>(request_);
    req.http_version_major = atoi(temp_.c_str());
    return true;
  }
  void onMajorFinish(const char& c) {
    temp_.clear();
    buffer_ = &(temp_);
  }

  bool isValidMinor(const char& c) const {
    if ('\r' != c) return false;

    auto req = const_cast<decltype(request_)&>(request_);
    req.http_version_minor = atoi(temp_.c_str());
    return true;
  }
  void onMinorFinish(const char& c) {
    temp_.clear();
    buffer_ = &(temp_);
  }

  void onHeaderLineStart(const char& c) {
    temp_.clear();
    buffer_ = &(temp_);
  }
  void onHeaderLineFinish(const char& c) {
    std::vector<std::string> parts =
        absl::StrSplit(*buffer_, absl::MaxSplits(":", 1));
    auto name = base::util::trim(parts[0]);
    if (name.size() > 0) {
      std::string value = parts.size() > 1 ? parts[1] : "";
      request_.headerList.add(name, base::util::trim(value));
    } else {
      // value is meanless without header name
    }
  }

  bool isNeedBody(const char& c) const {
    auto req = const_cast<decltype(request_)&>(request_);
    int bodySize = std::atoi(req.headerList.contentLength().c_str());
    return ('\n' == c && request_.method == Request::POST && bodySize > 0);
  }
  void onBodyStart(const char& c) {
    temp_.clear();
    buffer_ = &(temp_);
  }

  bool needMoreBody(const char& c) const {
    auto req = const_cast<decltype(request_)&>(request_);
    int bodySize = std::atoi(req.headerList.contentLength().c_str());
    return (buffer_->size() + 1 < bodySize);
  }

  void onBodyFinish(const char& c) {
    buffer_->append(1, c);
    request_.setBody(
        *buffer_,
        request_.headerList.get("Content-Type").value_or("text/plain"));
    buffer_->clear();
  }

 private:
  using fsm = HttpRequestParser_fsm;
  // clang-format off
  using transition_table = table<
    mem_fn_row<Start, char, Method, &fsm::appendToBuffer, &fsm::isAlpha>,
    mem_fn_row<Start, char, Error, &fsm::errorAction>,
    mem_fn_row<Method, char, Method, &fsm::appendToBuffer, &fsm::isAlpha>,
    mem_fn_row<Method, char, Uri, &fsm::onMethodFinish, &fsm::isValidMethod>,
    mem_fn_row<Method, char, Error, &fsm::errorAction>,
    mem_fn_row<Uri, char, Uri, &fsm::appendToBuffer, &fsm::isValidUriChar>,
    mem_fn_row<Uri, char, Http, &fsm::onUriFinish, &fsm::isSpace>,
    mem_fn_row<Uri, char, Error, &fsm::errorAction>,
    mem_fn_row<Http, char, Http, &fsm::appendToBuffer, &fsm::isAlpha>,
    mem_fn_row<Http, char, Major, &fsm::onHttpFinish, &fsm::isValidHttp>,
    mem_fn_row<Http, char, Error, &fsm::errorAction>,
    mem_fn_row<Major, char, Major, &fsm::appendToBuffer, &fsm::isDigit>,
    mem_fn_row<Major, char, Minor, &fsm::onMajorFinish, &fsm::isValidMajor>,
    mem_fn_row<Major, char, Error, &fsm::errorAction>,
    mem_fn_row<Minor, char, Minor, &fsm::appendToBuffer, &fsm::isDigit>,
    mem_fn_row<Minor, char, Expecting_newline_1, &fsm::onMinorFinish, &fsm::isValidMinor>,
    mem_fn_row<Minor, char, Error, &fsm::errorAction>,
    mem_fn_row<Expecting_newline_1, char, Header_line_start, &fsm::onHeaderLineStart, &fsm::isNewLine>,
    mem_fn_row<Expecting_newline_1, char, Error, &fsm::errorAction>,
    mem_fn_row<Header_line_start, char, Expecting_newline_3, &fsm::noAction, &fsm::isCarryReturn>,
    mem_fn_row<Header_line_start, char, Header_line, &fsm::appendToBuffer, &fsm::isAlpha>,
    mem_fn_row<Header_line_start, char, Error, &fsm::errorAction>,
    mem_fn_row<Header_line, char, Expecting_newline_2, &fsm::noAction, &fsm::isCarryReturn>,
    mem_fn_row<Header_line, char, Error, &fsm::errorAction, &fsm::isNewLine>,
    mem_fn_row<Header_line, char, Header_line, &fsm::appendToBuffer>,
    mem_fn_row<Expecting_newline_2, char, Header_line_start, &fsm::onHeaderLineFinish, &fsm::isNewLine>,
    mem_fn_row<Expecting_newline_2, char, Error, &fsm::errorAction>,
    mem_fn_row<Expecting_newline_3, char, Expecting_body, &fsm::onBodyStart, &fsm::isNeedBody>,
    mem_fn_row<Expecting_newline_3, char, Finish, &fsm::noAction, &fsm::isNewLine>,
    mem_fn_row<Expecting_newline_3, char, Error, &fsm::errorAction>,
    mem_fn_row<Expecting_body, char, Expecting_body, &fsm::appendToBuffer, &fsm::needMoreBody>,
    mem_fn_row<Expecting_body, char, Finish, &fsm::onBodyFinish>
  >;
  // clang-format on
};

int main(int argc, char const* argv[]) {
#if 0
  Request request("/get");
  request.headerList.setHost("httpbin.org");
  request.headerList.setConnectionClose();
#else
  Request request("/post", Request::POST);
  request.headerList.setHost("httpbin.org");
  request.headerList.setConnectionClose();
  request.setJSONBody("{}");
#endif

  std::string reqBuf = RequestFormatter::format(request);
  std::cout << "-------" << std::endl;
  std::cout << reqBuf << std::endl;
  std::cout << "-------" << std::endl;

  bool error = false;
  HttpRequestParser_fsm fsm;
  for (auto c : reqBuf) {
    fsm.process_event(c);
    if (fsm.current_state() == HttpRequestParser_fsm::Error) {
      error = true;
      break;
    }
    if (fsm.current_state() == HttpRequestParser_fsm::Finish) {
      break;
    }
  }

  std::cout << "error: " << error << std::endl;
  std::cout << "method: " << request.method << std::endl;
  std::cout << "uri: " << request.uri << std::endl;
  std::cout << "major: " << request.http_version_major << std::endl;
  std::cout << "minor: " << request.http_version_minor << std::endl;
  std::cout << "headers: " << std::endl;
  for (const auto& hdr : request.headerList.headers) {
    std::cout << "  " << hdr.name << ": " << hdr.value << std::endl;
  }
  if (request.method == Request::POST) {
    std::cout << "body: " << std::endl;
    std::cout << request.body() << std::endl;
  }

  return 0;
}
