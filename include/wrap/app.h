#pragma once

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

  folly::dynamic json() const { return folly::parseJson(body()); }

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
struct IdPolicy {
  static std::string generate() { return fmt::format("{}", folly::Random::rand64()); }
};

template <class Owner, class Member>
struct field_t {
  std::string_view name;
  Member Owner::* ptr;
};

template <class Owner, class Member>
constexpr field_t<Owner, Member> field(std::string_view name, Member Owner::* ptr) {
  return {name, ptr};
}

template <typename T>
class Model {
public:
  static std::optional<T> parse(std::string const& str) {
    static auto const validator = folly::jsonschema::makeValidator(T::schema());
    try {
      auto const json = folly::parseJson(str);
      validator->validate(json);
      auto obj = T::make(json);
      auto self = static_cast<Model*>(&(*obj));
      if (json.count("_id")) {
        self->id_ = json["_id"].asString();
      } else {
        self->id_ = IdPolicy<T>::generate();
      }
      return obj;
    } catch (...) {
      return std::nullopt;
    }
  }

  std::string const& _id() const { return id_; }

protected:
  std::string id_;
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

  template <typename T, typename F>
  App& post(std::string const& path, F&& func) {
    return post(path, [f = std::forward<F>(func)](Request const& req, Response& res) {
      auto const obj = T::parse(req.body());
      if (!obj) {
        res.status(422, "Unprocessable Entity").body(R"({"error":"Could not parse request data"})");
        return;
      }
      try {
        f(*obj);
      } catch (...) {
        res.status(500, "Internal Server Error")
            .body(R"({"error":"Error processing the request"})");
        return;
      }
      res.status(201, "Created")
          .header("Location", obj->_id())
          .header("Content-Type", "application/json")
          .body(folly::toJson(obj->dump()));
    });
  }

  void run(std::string const& host, std::uint16_t port);

  void run();

private:
  AppOptions options_;
  std::unique_ptr<proxygen::HTTPServer> server_;
  std::vector<Route> routes_;
};
}  // namespace wrap
