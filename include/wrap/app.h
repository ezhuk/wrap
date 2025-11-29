#pragma once

#include <cstdint>
#include <string>

namespace wrap {
class AppOptions {
public:
  std::string host{"0.0.0.0"};
  std::uint16_t port{8080};
  std::size_t threads{0};
};

class App final {
public:
  explicit App(AppOptions options = {});
  virtual ~App();

private:
  AppOptions options_;
};
}  // namespace wrap
