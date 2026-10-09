#pragma once

#include <folly/coro/Task.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace proxygen::coro {
class HTTPServer;
}

namespace wrap {
template <typename T>
using Task = folly::coro::Task<T>;

struct AppOptions {
  std::size_t threads = 0;
};

class App final {
public:
  using GetHandler = std::function<Task<std::string>()>;

  explicit App(AppOptions options = {});
  ~App();

  App& get(std::string path, GetHandler handler);

  template <typename F>
    requires requires(F& handler) {
      { handler() } -> std::convertible_to<std::string>;
    }
  App& get(std::string path, F handler) {
    return get(std::move(path), [handler = std::move(handler)]() mutable -> Task<std::string> {
      co_return std::string{handler()};
    });
  }

  void run(std::string host = "127.0.0.1", std::uint16_t port = 8080);
  void stop();

private:
  class Handler;

  std::uint16_t port_;
  std::size_t threads_;

  AppOptions options_;
  std::shared_ptr<Handler> handler_;
  std::unique_ptr<proxygen::coro::HTTPServer> server_;
};
}  // namespace wrap
