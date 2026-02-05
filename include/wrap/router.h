#pragma once

#include "wrap/app.h"
#include "wrap/handler.h"
#include "wrap/middleware.h"

namespace wrap {
class Router final {
public:
  Router(App& app, std::string prefix) : app_(app), prefix_(normalize_prefix(std::move(prefix))) {}

  Router& use(Middleware middleware) {
    app_.use(std::move(middleware));
    return *this;
  }

  Router& get(std::string const& path, Handler handler) {
    app_.get(join(path), std::move(handler));
    return *this;
  }

  Router& post(std::string const& path, Handler handler) {
    app_.post(join(path), std::move(handler));
    return *this;
  }

  Router& put(std::string const& path, Handler handler) {
    app_.put(join(path), std::move(handler));
    return *this;
  }

  template <typename F>
  Router& get(std::string const& path, F&& func) {
    app_.get(join(path), std::forward<F>(func));
    return *this;
  }

private:
  static std::string normalize_prefix(std::string p) {
    if (p.empty()) {
      return "";
    }
    if (p.front() != '/') {
      p.insert(p.begin(), '/');
    }
    while (p.size() > 1 && p.back() == '/') {
      p.pop_back();
    }
    return p;
  }

  std::string join(std::string const& path) const {
    if (path.empty()) {
      return prefix_;
    }
    if (path.front() == '/') {
      if (prefix_.empty()) {
        return path;
      }
      return prefix_ + path;
    }
    if (prefix_.empty()) {
      return "/" + path;
    }
    return prefix_ + "/" + path;
  }

private:
  App& app_;
  std::string prefix_;
};
}  // namespace wrap
