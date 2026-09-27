#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace wrap {
class Response final {
public:
  using Header = std::pair<std::string, std::string>;

  explicit Response(std::uint16_t status = 200) : status_(status) {}

  static Response text(std::string body, std::uint16_t status = 200) {
    Response response(status);

    response.header("Content-Type", "text/plain; charset=utf-8").body(std::move(body));

    return response;
  }

  Response& status(std::uint16_t status) noexcept {
    status_ = status;
    return *this;
  }

  Response& header(std::string name, std::string value) {
    headers_.emplace_back(std::move(name), std::move(value));

    return *this;
  }

  Response& body(std::string body) {
    body_ = std::move(body);
    return *this;
  }

  [[nodiscard]]
  std::uint16_t status() const noexcept {
    return status_;
  }

  [[nodiscard]]
  std::string_view body() const noexcept {
    return body_;
  }

  [[nodiscard]]
  std::vector<Header> const& headers() const noexcept {
    return headers_;
  }

private:
  std::uint16_t status_{200};

  std::vector<Header> headers_;
  std::string body_;
};
}  // namespace wrap
