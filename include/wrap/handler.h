#pragma once

#include <functional>

#include "wrap/request.h"
#include "wrap/response.h"

namespace wrap {
using Handler = std::function<void(Request const&, Response&)>;
}  // namespace wrap
