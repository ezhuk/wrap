#pragma once

#include <proxygen/lib/http/HTTPMessage.h>

#include <unordered_map>

namespace wrap {
class Request final {
public:
  Request(proxygen::HTTPMessage const* msg, folly::IOBuf* body) : msg_(msg), body_(body) {}
  ~Request() = default;

  std::string getMethod() const { return msg_->getMethodString(); }

  std::string getURL() const { return msg_->getURL(); }

  std::string getParam(std::string const& name) const {
    auto iter = params_.find(name);
    if (params_.end() != iter) {
      return iter->second;
    }
    return "";
  }

  void setParam(std::string const& name, std::string const& data) { params_[name] = data; }

  std::string getQueryParam(std::string const& name) const {
    return msg_->getDecodedQueryParam(name);
  }

  std::string body() const { return body_ ? body_->toString() : std::string{}; }

  folly::dynamic json() const { return folly::parseJson(body()); }

private:
  proxygen::HTTPMessage const* msg_;
  folly::IOBuf* body_;
  std::unordered_map<std::string, std::string> params_;
};
}  // namespace wrap
