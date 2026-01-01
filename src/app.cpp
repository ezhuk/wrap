#include "wrap/app.h"

#include <folly/String.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>
#include <proxygen/httpserver/filters/DirectResponseHandler.h>

namespace wrap {
namespace {
static folly::StringPiece normalize(folly::StringPiece str) {
  while (str.size() > 1 && str.back() == '/') {
    str.pop_back();
  }
  return str;
}

class RequestHandler final : public proxygen::RequestHandler {
public:
  explicit RequestHandler(
      std::vector<App::Route>* routes, std::vector<App::Middleware>* middlewares
  )
      : routes_(routes), middlewares_(middlewares) {}

  void onRequest(std::unique_ptr<proxygen::HTTPMessage> request) noexcept override {
    request_ = std::move(request);
  }

  void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override {
    if (body_) {
      body_->prependChain(std::move(body));
    } else {
      body_ = std::move(body);
    }
  }

  void onEOM() noexcept override {
    auto request = Request(request_.get(), body_.get());
    auto handler = getHandler(request);
    if (handler) {
      proxygen::ResponseBuilder builder(downstream_);
      Response response(&builder);
      auto next = handler;
      for (auto iter = middlewares_->rbegin(); iter != middlewares_->rend(); ++iter) {
        next = (*iter)(std::move(next));
      }
      next(request, response);
      if (builder.getHeaders() && builder.getHeaders()->getStatusCode()) {
        builder.sendWithEOM();
      } else {
        proxygen::ResponseBuilder(downstream_)
            .status(404, "Not Found")
            .body("{\"error\":\"Not Found\"}")
            .sendWithEOM();
      }
    } else {
      proxygen::ResponseBuilder(downstream_)
          .status(404, "Not Found")
          .body("{\"error\":\"Not Found\"}")
          .sendWithEOM();
    }
  }

  void onUpgrade(proxygen::UpgradeProtocol) noexcept override {}

  void requestComplete() noexcept override { delete this; }

  void onError(proxygen::ProxygenError) noexcept override { delete this; }

private:
  App::Handler getHandler(Request& request) {
    std::vector<folly::StringPiece> parts;
    folly::split('/', normalize(request_->getPathAsStringPiece()), parts);
    for (auto const& route : *routes_) {
      if (route.method == request_->getMethod()) {
        std::vector<folly::StringPiece> segments;
        folly::split('/', normalize(route.path), segments);
        if (parts.size() == segments.size()) {
          bool match = true;
          std::unordered_map<std::string, std::string> params;
          for (std::size_t i = 0; i < parts.size(); ++i) {
            auto lhs = parts[i];
            auto rhs = segments[i];
            if (!rhs.empty() && rhs.front() == ':') {
              if (lhs.empty()) {
                match = false;
                break;
              }
              params.emplace(rhs.subpiece(1).str(), lhs.str());
              continue;
            }
            if (rhs.size() >= 3 && rhs.front() == '{' && rhs.back() == '}') {
              if (lhs.empty()) {
                match = false;
                break;
              }
              auto inner = rhs.subpiece(1, rhs.size() - 2);
              folly::StringPiece name = inner;
              folly::StringPiece type;
              auto colon = inner.find(':');
              if (colon != folly::StringPiece::npos) {
                name = inner.subpiece(0, colon);
                type = inner.subpiece(colon + 1);
              }
              if (name.empty()) {
                match = false;
                break;
              }
              if (!type.empty()) {
                if (type == "int") {
                  bool ok = !lhs.empty();
                  for (char c : lhs) {
                    if (c < '0' || c > '9') {
                      ok = false;
                      break;
                    }
                  }
                  if (!ok) {
                    match = false;
                    break;
                  }
                } else if (type == "string") {
                  // always valid
                } else {
                  match = false;
                  break;
                }
              }
              params.emplace(name.str(), lhs.str());
              continue;
            }
            if (lhs != rhs) {
              match = false;
              break;
            }
          }

          if (match) {
            for (auto& [k, v] : params) {
              request.setParam(k, v);
            }
            return route.handler;
          }
        }
      }
    }
    return nullptr;
  }

private:
  std::vector<App::Route>* routes_;
  std::vector<App::Middleware>* middlewares_;
  std::unique_ptr<proxygen::HTTPMessage> request_;
  std::unique_ptr<folly::IOBuf> body_;
};

class HandlerFactory final : public proxygen::RequestHandlerFactory {
public:
  explicit HandlerFactory(
      std::vector<App::Route>* routes, std::vector<App::Middleware>* middlewares
  )
      : routes_(routes), middlewares_(middlewares) {}

  void onServerStart(folly::EventBase*) noexcept override {}

  void onServerStop() noexcept override {}

  proxygen::RequestHandler* onRequest(
      proxygen::RequestHandler*, proxygen::HTTPMessage*
  ) noexcept override {
    return new RequestHandler(routes_, middlewares_);
  }

private:
  std::vector<App::Route>* routes_;
  std::vector<App::Middleware>* middlewares_;
};
}  // namespace

App::App(AppOptions options) : options_(std::move(options)) {}

App& App::post(std::string const& path, Handler handler) {
  routes_.push_back(Route{proxygen::HTTPMethod::POST, path, std::move(handler)});
  return *this;
}

App& App::put(std::string const& path, Handler handler) {
  routes_.push_back(Route{proxygen::HTTPMethod::PUT, path, std::move(handler)});
  return *this;
}

App& App::get(std::string const& path, Handler handler) {
  routes_.push_back(Route{proxygen::HTTPMethod::GET, path, std::move(handler)});
  return *this;
}

void App::run(std::string const& host, std::uint16_t port) {
  options_.host = host;
  options_.port = port;
  run();
}

void App::run() {
  proxygen::HTTPServerOptions options;
  options.threads = options_.threads;
  options.handlerFactories =
      proxygen::RequestHandlerChain().addThen<HandlerFactory>(&routes_, &middlewares_).build();
  server_ = std::make_unique<proxygen::HTTPServer>(std::move(options));
  server_->bind(
      {{folly::SocketAddress(options_.host, options_.port, true),
        proxygen::HTTPServer::Protocol::HTTP}}
  );
  server_->start();
  server_->stop();
  server_.reset();
}
}  // namespace wrap
