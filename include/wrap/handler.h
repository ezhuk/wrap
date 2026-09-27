#pragma once

#include <folly/coro/Task.h>

#include <functional>

#include "wrap/request.h"
#include "wrap/response.h"

namespace wrap {
using Handler = std::function<folly::coro::Task<Response>(Request const&)>;
}  // namespace wrap
