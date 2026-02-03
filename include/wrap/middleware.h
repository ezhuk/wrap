#pragma once

#include "wrap/handler.h"

namespace wrap {
using Middleware = std::function<Handler(Handler)>;

namespace middleware {
inline Middleware logger() {
  return [](Handler next) {
    return [next = std::move(next)](Request const& req, Response& res) {
      next(req, res);
      fmt::print("{} {}\n", req.getMethod(), req.getURL());
    };
  };
}
}  // namespace middleware
}  // namespace wrap
