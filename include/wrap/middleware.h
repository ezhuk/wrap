#pragma once

#include "app.h"

namespace wrap{
namespace middleware {
inline App::Middleware logger() {
  return [](App::Handler next) {
    return [next = std::move(next)](Request const& req, Response& res) {
      next(req, res);
      fmt::print("{} {}\n", req.getMethod(), req.getURL());
    };
  };
}
}  // namespace middleware
}  // namespace wrap
