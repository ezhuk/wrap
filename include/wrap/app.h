#pragma once

#include <folly/json/DynamicConverter.h>
#include <folly/json/JSONSchema.h>
#include <folly/json/json.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/ResponseBuilder.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

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

  folly::dynamic json() const { return folly::parseJson(body_->toString()); }

private:
  proxygen::HTTPMessage const* msg_;
  folly::IOBuf* body_;
  std::unordered_map<std::string, std::string> params_;
};

class Response final {
public:
  explicit Response(proxygen::ResponseBuilder* res) : res_(res) {}
  ~Response() = default;

  Response& status(std::uint16_t code, std::string const& message) {
    res_->status(code, message);
    return *this;
  }

  Response& header(std::string const& name, std::string const& data) {
    res_->header(name, data);
    return *this;
  }

  Response& body(std::string const& body) {
    res_->body(body);
    return *this;
  }

private:
  proxygen::ResponseBuilder* res_;
};

template <typename T>
class Model {
public:
  static std::optional<T> parse(std::string const& str) {
    static auto const validator = folly::jsonschema::makeValidator(T::schema());
    try {
      auto const json = folly::parseJson(str);
      validator->validate(json);
      return folly::convertTo<T>(json);
    } catch (...) {
      return std::nullopt;
    }
  }
};

class AppOptions {
public:
  std::string host{"0.0.0.0"};
  std::uint16_t port{8080};
  std::size_t threads{0};
};

class App final {
public:
  using Handler = std::function<void(Request const&, Response&)>;

  struct Route {
    proxygen::HTTPMethod method;
    std::string path;
    Handler handler;
  };

  explicit App(AppOptions options = {});
  ~App() = default;

  App& post(std::string const& path, Handler handler);
  App& get(std::string const& path, Handler handler);

  void run(std::string const& host, std::uint16_t port);

  void run();

private:
  AppOptions options_;
  std::unique_ptr<proxygen::HTTPServer> server_;
  std::vector<Route> routes_;
};
}  // namespace wrap
