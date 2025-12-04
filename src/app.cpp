#include "wrap/app.h"

#include <folly/String.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>
#include <proxygen/httpserver/filters/DirectResponseHandler.h>

namespace wrap {
namespace {
class RequestHandler final : public proxygen::RequestHandler {
public:
  explicit RequestHandler(std::vector<App::Route>* routes) : routes_(routes) {}

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
    auto request = Request(request_.get());
    auto handler = getHandler(request);
    if (handler) {
      proxygen::ResponseBuilder builder(downstream_);
      Response response(&builder);
      handler(request, response);
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
    auto const method = request_->getMethod();
    auto const url = request_->getURL();

    std::vector<folly::StringPiece> parts;
    folly::split('/', url, parts);

    for (auto const& route : *routes_) {
      if (route.method != method) {
        continue;
      }

      folly::StringPiece routePath(route.path);
      std::vector<folly::StringPiece> routeParts;
      folly::split('/', routePath, routeParts);

      if (routeParts.size() != parts.size()) {
        continue;
      }

      bool matched = true;
      std::unordered_map<std::string, std::string> params;

      for (std::size_t i = 0; i < routeParts.size(); ++i) {
        auto seg = routeParts[i];
        auto val = parts[i];
        if (!seg.empty() && seg.front() == ':') {
          if (val.empty()) {
            matched = false;
            break;
          }
          auto name = seg.subpiece(1);
          params.emplace(name.str(), val.str());
        } else {
          if (seg != val) {
            matched = false;
            break;
          }
        }
      }
      if (matched) {
        for (auto& [k, v] : params) {
          request.setParam(k, v);
        }
        return route.handler;
      }
    }
    return nullptr;
  }

private:
  std::vector<App::Route>* routes_;
  std::unique_ptr<proxygen::HTTPMessage> request_;
  std::unique_ptr<folly::IOBuf> body_;
};

class HandlerFactory final : public proxygen::RequestHandlerFactory {
public:
  explicit HandlerFactory(std::vector<App::Route>* routes) : routes_(routes) {}

  void onServerStart(folly::EventBase*) noexcept override {}

  void onServerStop() noexcept override {}

  proxygen::RequestHandler* onRequest(
      proxygen::RequestHandler*, proxygen::HTTPMessage*
  ) noexcept override {
    return new RequestHandler(routes_);
  }

private:
  std::vector<App::Route>* routes_;
};
}  // namespace

App::App(AppOptions options) : options_(std::move(options)) {}

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
      proxygen::RequestHandlerChain().addThen<HandlerFactory>(&routes_).build();
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
