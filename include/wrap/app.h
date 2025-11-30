#pragma once

#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/ResponseBuilder.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace wrap {
class AppOptions {
public:
  std::string host{"0.0.0.0"};
  std::uint16_t port{8080};
  std::size_t threads{0};
};

class App final {
public:
  using Handler = std::function<void(proxygen::HTTPMessage const&, proxygen::ResponseBuilder&)>;

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
