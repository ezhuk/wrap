#pragma once

#include <proxygen/httpserver/ResponseBuilder.h>

namespace wrap {
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
}  // namespace wrap
