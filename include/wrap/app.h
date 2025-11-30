#pragma once

#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/ResponseBuilder.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace wrap {
class Request final {
public:
  explicit Request(proxygen::HTTPMessage const* msg) : msg_(msg) {}
  ~Request() = default;

  std::string getMethod() const { return msg_->getMethodString(); }

  std::string getURL() const { return msg_->getURL(); }

private:
  proxygen::HTTPMessage const* msg_;
};

class AppOptions {
public:
  std::string host{"0.0.0.0"};
  std::uint16_t port{8080};
  std::size_t threads{0};
};

class App final {
public:
  using Handler = std::function<void(Request const&, proxygen::ResponseBuilder&)>;

  explicit App(AppOptions options = {});
  ~App() = default;

  App& get(std::string const& path, Handler handler);

  void run(std::string const& host, std::uint16_t port);

  void run();

private:
  AppOptions options_;
  std::unique_ptr<proxygen::HTTPServer> server_;
  std::unordered_map<std::string, Handler> handlers_;
};
}  // namespace wrap
