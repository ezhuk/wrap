#pragma once

#include <string>
#include <string_view>
#include <utility>

namespace wrap {
class Request final {
public:
  Request(std::string method, std::string path)
      : method_(std::move(method)), path_(std::move(path)) {}

  [[nodiscard]]
  std::string_view method() const noexcept {
    return method_;
  }

  [[nodiscard]]
  std::string_view path() const noexcept {
    return path_;
  }

private:
  std::string method_;
  std::string path_;
};
}  // namespace wrap
