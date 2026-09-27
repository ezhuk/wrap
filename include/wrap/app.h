#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "wrap/handler.h"
#include "wrap/middleware.h"

namespace wrap {
struct AppOptions {
  std::string host{"0.0.0.0"};

  std::uint16_t port{8080};

  std::size_t threads{1};
};

class App final {
public:
  explicit App(AppOptions options = {});

  ~App();

  App(App const&) = delete;
  App& operator=(App const&) = delete;

  App(App&&) = delete;
  App& operator=(App&&) = delete;

  App& use(Middleware middleware);

  App& get(std::string path, Handler handler);

  void run();

  void stop();

private:
  struct Impl;

  std::unique_ptr<Impl> impl_;
};
}  // namespace wrap
