#pragma once

#include <folly/coro/Task.h>

#include <functional>
#include <string>
#include <utility>

#include "wrap/handler.h"

namespace wrap {
using Middleware = std::function<Handler(Handler)>;

namespace middleware {
inline Middleware header(std::string name, std::string value) {
  return [name = std::move(name), value = std::move(value)](Handler next) mutable {
    return [next = std::move(next), name = std::move(name),
            value = std::move(value)](Request const& request) -> folly::coro::Task<Response> {
      auto response = co_await next(request);

      response.header(name, value);

      co_return response;
    };
  };
}
}  // namespace middleware
}  // namespace wrap
