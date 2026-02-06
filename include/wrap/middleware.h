#pragma once

#include <atomic>

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

inline Middleware tracer(std::string prefix = {}) {
  static std::atomic<uint64_t> counter{1};
  return [prefix = std::move(prefix)](Handler next) {
    return [next = std::move(next), prefix](Request const& req, Response& res) {
      res.header(
          "X-Request-Id", prefix + std::to_string(counter.fetch_add(1, std::memory_order_relaxed))
      );
      next(req, res);
    };
  };
}
}  // namespace middleware
}  // namespace wrap
