#pragma once

#include <folly/json/json.h>
#include <proxygen/httpserver/HTTPServer.h>

#include <memory>
#include <vector>

#include "wrap/handler.h"
#include "wrap/middleware.h"
#include "wrap/request.h"
#include "wrap/response.h"

namespace wrap {
namespace detail {
inline void send_json(
    Response& res, std::uint16_t code, std::string const& message, folly::dynamic const& body
) {
  res.status(code, message).header("Content-Type", "application/json").body(folly::toJson(body));
}

inline void send_error(Response& res, std::uint16_t code, std::string const& detail) {
  send_json(
      res, code, (code == 404 ? "Not Found" : "Error"), folly::dynamic::object("detail", detail)
  );
}

inline void send_ok(Response& res, std::string_view body) {
  res.status(200, "OK").header("Content-Type", "text/plain; charset=utf-8").body(std::string(body));
}

inline void send_no_content(Response& res) { res.status(204, "No Content"); }

inline std::vector<std::string> braced_param_names(std::string const& path) {
  std::vector<std::string> out;
  std::size_t pos = 0;
  while (true) {
    auto l = path.find('{', pos);
    if (l == std::string::npos) {
      break;
    }
    auto r = path.find('}', l + 1);
    if (r == std::string::npos) {
      break;
    }
    folly::StringPiece inner(path.data() + l + 1, r - (l + 1));
    auto colon = inner.find(':');
    if (colon != folly::StringPiece::npos) {
      inner = inner.subpiece(0, colon);
    }
    if (!inner.empty()) {
      out.push_back(inner.str());
    }
    pos = r + 1;
  }
  return out;
}

template <class T>
bool convert_param(std::string const& s, T& out) {
  using U = std::remove_cvref_t<T>;
  if constexpr (std::is_same_v<U, std::string>) {
    out = s;
    return true;
  } else if constexpr (std::is_integral_v<U> && !std::is_same_v<U, bool>) {
    if (s.empty()) {
      return false;
    }
    std::uint64_t v = 0;
    for (char c : s) {
      if (c < '0' || c > '9') {
        return false;
      }
      std::uint64_t d = static_cast<std::uint64_t>(c - '0');
      if (v > (std::numeric_limits<std::uint64_t>::max() - d) / 10) {
        return false;
      }
      v = v * 10 + d;
    }
    if constexpr (std::is_signed_v<U>) {
      if (v > static_cast<std::uint64_t>(std::numeric_limits<U>::max())) {
        return false;
      }
      out = static_cast<U>(v);
    } else {
      if (v > static_cast<std::uint64_t>(std::numeric_limits<U>::max())) {
        return false;
      }
      out = static_cast<U>(v);
    }
    return true;
  } else {
    static_assert(sizeof(T) == 0, "Unsupported path param type");
  }
}
}  // namespace detail

class AppOptions {
public:
  std::string host{"0.0.0.0"};
  std::uint16_t port{8080};
  std::size_t threads{0};
};

class App final {
public:
  struct Route {
    proxygen::HTTPMethod method;
    std::string path;
    Handler handler;
  };

  explicit App(AppOptions options = {});
  ~App() = default;

  App& use(Middleware middleware) {
    middlewares_.push_back(std::move(middleware));
    return *this;
  }

  App& post(std::string const& path, Handler handler);
  App& put(std::string const& path, Handler handler);
  App& get(std::string const& path, Handler handler);

  template <class F>
  App& get(std::string const& path, F&& func) {
    auto const names = detail::braced_param_names(path);
    Handler h = [f = std::forward<F>(func), names](Request const& req, Response& res) mutable {
      try {
        auto call_and_respond = [&](auto&&... args) {
          using Ret = std::invoke_result_t<decltype(f)&, decltype(args)...>;
          if constexpr (std::is_void_v<Ret>) {
            std::invoke(f, std::forward<decltype(args)>(args)...);
            detail::send_no_content(res);
          } else {
            auto out = std::invoke(f, std::forward<decltype(args)>(args)...);
            if constexpr (std::is_convertible_v<decltype(out), std::string_view>) {
              detail::send_ok(res, std::string_view(out));
            } else if constexpr (std::is_same_v<
                                     std::remove_cvref_t<decltype(out)>, folly::dynamic>) {
              detail::send_json(res, 200, "OK", out);
            } else {
              static_assert(sizeof(out) == 0, "Unsupported handler return type");
            }
          }
        };
        if constexpr (std::invocable<decltype(f)&>) {
          if (!names.empty()) {
            detail::send_error(res, 500, "Internal Server Error");
            return;
          }
          call_and_respond();
          return;
        }
        if constexpr (std::invocable<decltype(f)&, int>) {
          if (names.size() != 1) {
            detail::send_error(res, 500, "Internal Server Error");
            return;
          }
          int v{};
          if (!detail::convert_param(req.getParam(names[0]), v)) {
            detail::send_error(res, 404, "Not Found");
            return;
          }
          call_and_respond(v);
          return;
        }
        if constexpr (std::invocable<decltype(f)&, std::string>) {
          if (names.size() != 1) {
            detail::send_error(res, 500, "Internal Server Error");
            return;
          }
          std::string v{};
          if (!detail::convert_param(req.getParam(names[0]), v)) {
            detail::send_error(res, 404, "Not Found");
            return;
          }
          call_and_respond(std::move(v));
          return;
        }
        detail::send_error(res, 500, "Internal Server Error");
      } catch (...) {
        detail::send_error(res, 500, "Internal Server Error");
      }
    };
    return get(path, std::move(h));
  }

  void run(std::string const& host, std::uint16_t port);
  void run();

  void stop();

private:
  AppOptions options_;
  std::unique_ptr<proxygen::HTTPServer> server_;
  std::vector<Route> routes_;
  std::vector<Middleware> middlewares_;
};
}  // namespace wrap
