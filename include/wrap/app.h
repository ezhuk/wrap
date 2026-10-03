#pragma once

#include <folly/coro/Task.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace proxygen::coro {
class HTTPServer;
}

namespace wrap {
class App final {
public:
  using GetHandler = std::function<folly::coro::Task<std::string>()>;

  explicit App(std::uint16_t port = 8080, std::size_t threads = 1);
  ~App();

  App(App const&) = delete;
  App& operator=(App const&) = delete;

  App(App&&) = delete;
  App& operator=(App&&) = delete;

  App& get(std::string path, GetHandler handler);

  void run();
  void stop();

private:
  class Handler;

  std::uint16_t port_;
  std::size_t threads_;

  std::shared_ptr<Handler> handler_;

  std::unique_ptr<proxygen::coro::HTTPServer> server_;
};
}  // namespace wrap
