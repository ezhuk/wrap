#include "wrap/app.h"

#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/coro/HTTPCoroSession.h>
#include <proxygen/lib/http/coro/HTTPFixedSource.h>
#include <proxygen/lib/http/coro/server/HTTPServer.h>

#include <stdexcept>
#include <utility>
#include <vector>

namespace wrap {
class App::Handler final : public proxygen::coro::HTTPHandler {
public:
  void addRoute(std::string path, GetHandler handler) {
    for (auto const& route : routes_) {
      if (route.path == path) {
        throw std::invalid_argument("duplicate GET route: " + path);
      }
    }

    routes_.push_back({
        .path = std::move(path),
        .handler = std::move(handler),
    });
  }

  folly::coro::Task<proxygen::coro::HTTPSourceHolder> handleRequest(
      folly::EventBase*, proxygen::coro::HTTPSessionContextPtr,
      proxygen::coro::HTTPSourceHolder request
  ) override {
    auto event = co_await request.readHeaderEvent();

    auto& message = *event.headers;

    if (!event.eom) {
      request.stopReading();
    }

    if (message.getMethod() != proxygen::HTTPMethod::GET) {
      co_return proxygen::coro::HTTPFixedSource::makeFixedResponse(404, "Not Found\n");
    }

    auto path = message.getPathAsStringPiece();

    for (auto const& route : routes_) {
      if (path == route.path) {
        auto body = co_await route.handler();

        co_return proxygen::coro::HTTPFixedSource::makeFixedResponse(200, std::move(body));
      }
    }

    co_return proxygen::coro::HTTPFixedSource::makeFixedResponse(404, "Not Found\n");
  }

private:
  struct Route {
    std::string path;
    GetHandler handler;
  };

  std::vector<Route> routes_;
};

App::App(std::uint16_t port, std::size_t threads)
    : port_(port), threads_(threads), handler_(std::make_shared<Handler>()) {}

App::~App() { stop(); }

App& App::get(std::string path, GetHandler handler) {
  handler_->addRoute(std::move(path), std::move(handler));
  return *this;
}

void App::run() {
  proxygen::coro::HTTPServer::Config config;
  config.socketConfig.bindAddress.setFromLocalPort(port_);
  config.numIOThreads = threads_;
  config.shutdownOnSignals = {};

  server_ = std::make_unique<proxygen::coro::HTTPServer>(std::move(config), handler_);
  server_->start();
}

void App::stop() {
  if (server_) {
    server_->drain();
    server_->forceStop();
  }
}
}  // namespace wrap
