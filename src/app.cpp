#include "wrap/app.h"

#include <folly/SocketAddress.h>
#include <folly/io/IOBuf.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/coro/HTTPCoroSession.h>
#include <proxygen/lib/http/coro/HTTPFixedSource.h>
#include <proxygen/lib/http/coro/server/HTTPServer.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace wrap {
namespace {
struct Route {
  std::string path;
  Handler handler;
};

proxygen::coro::HTTPSourceHolder make_response(Response response) {
  auto message = std::make_unique<proxygen::HTTPMessage>();

  message->setHTTPVersion(1, 1);

  message->setStatusCode(response.status());

  message->setStatusMessage(proxygen::HTTPMessage::getDefaultReason(response.status()));

  for (auto const& [name, value] : response.headers()) {
    message->getHeaders().set(name, value);
  }

  std::unique_ptr<folly::IOBuf> body;

  if (!response.body().empty()) {
    body = folly::IOBuf::copyBuffer(response.body());
  }

  return proxygen::coro::HTTPFixedSource::makeFixedSource(std::move(message), std::move(body));
}

Response not_found() { return Response::text("Not Found\n", 404); }

Response method_not_allowed() {
  auto response = Response::text("Method Not Allowed\n", 405);

  response.header("Allow", "GET");

  return response;
}

Response internal_server_error() { return Response::text("Internal Server Error\n", 500); }

class Dispatcher final : public proxygen::coro::HTTPHandler {
public:
  explicit Dispatcher(std::vector<Route> const& routes) : routes_(routes) {}

  folly::coro::Task<proxygen::coro::HTTPSourceHolder> handleRequest(
      folly::EventBase*, proxygen::coro::HTTPSessionContextPtr,
      proxygen::coro::HTTPSourceHolder requestSource
  ) override {
    try {
      auto headerEvent = co_await requestSource.readHeaderEvent();

      auto const& message = *headerEvent.headers;

      auto const method = message.getMethodString();

      auto const path = message.getPathAsStringPiece().str();

      //
      // Body support is intentionally not
      // part of this initial version.
      //
      // If the request isn't already at EOM,
      // explicitly stop consuming it.
      //
      if (!headerEvent.eom) {
        requestSource.stopReading();
      }

      if (method != "GET") {
        co_return make_response(method_not_allowed());
      }

      Request request(method, path);

      auto const route =
          std::find_if(routes_.begin(), routes_.end(), [&request](Route const& route) {
            return route.path == request.path();
          });

      if (route == routes_.end()) {
        co_return make_response(not_found());
      }

      try {
        auto response = co_await route->handler(request);

        co_return make_response(std::move(response));
      } catch (...) {
        co_return make_response(internal_server_error());
      }
    } catch (...) {
      co_return make_response(internal_server_error());
    }
  }

private:
  std::vector<Route> const& routes_;
};

}  // namespace

struct App::Impl {
  explicit Impl(AppOptions options) : options(std::move(options)) {}

  AppOptions options;

  std::vector<Route> routes;
  std::vector<Middleware> middlewares;

  bool frozen{false};

  std::atomic<bool> running{false};

  std::mutex serverMutex;

  std::unique_ptr<proxygen::coro::HTTPServer> server;
};

App::App(AppOptions options) : impl_(std::make_unique<Impl>(std::move(options))) {}

App::~App() { stop(); }

App& App::use(Middleware middleware) {
  if (impl_->frozen) {
    throw std::logic_error(
        "middleware cannot be "
        "registered after run()"
    );
  }

  impl_->middlewares.push_back(std::move(middleware));

  return *this;
}

App& App::get(std::string path, Handler handler) {
  if (impl_->frozen) {
    throw std::logic_error(
        "routes cannot be "
        "registered after run()"
    );
  }

  impl_->routes.push_back({
      .path = std::move(path),
      .handler = std::move(handler),
  });

  return *this;
}

void App::run() {
  if (impl_->frozen) {
    throw std::logic_error(
        "App::run() may only "
        "be called once"
    );
  }

  //
  // Compile middleware once.
  //
  for (auto& route : impl_->routes) {
    auto handler = std::move(route.handler);

    for (auto middleware = impl_->middlewares.rbegin(); middleware != impl_->middlewares.rend();
         ++middleware) {
      handler = (*middleware)(std::move(handler));
    }

    route.handler = std::move(handler);
  }

  impl_->frozen = true;

  proxygen::coro::HTTPServer::Config config;

  config.socketConfig.bindAddress =
      folly::SocketAddress(impl_->options.host, impl_->options.port, true);

  config.numIOThreads = std::max<std::size_t>(1, impl_->options.threads);

  //
  // wrap controls shutdown itself.
  //
  config.shutdownOnSignals = {};

  auto dispatcher = std::make_shared<Dispatcher>(impl_->routes);

  {
    std::lock_guard lock(impl_->serverMutex);

    impl_->server =
        std::make_unique<proxygen::coro::HTTPServer>(std::move(config), std::move(dispatcher));
  }

  proxygen::coro::HTTPServer* server = nullptr;

  {
    std::lock_guard lock(impl_->serverMutex);

    server = impl_->server.get();
  }

  try {
    server->start([this] { impl_->running.store(true, std::memory_order_release); });
  } catch (...) {
    impl_->running.store(false, std::memory_order_release);

    std::lock_guard lock(impl_->serverMutex);

    impl_->server.reset();

    throw;
  }

  impl_->running.store(false, std::memory_order_release);

  {
    std::lock_guard lock(impl_->serverMutex);

    impl_->server.reset();
  }
}

void App::stop() {
  if (!impl_) {
    return;
  }

  if (!impl_->running.load(std::memory_order_acquire)) {
    return;
  }

  std::lock_guard lock(impl_->serverMutex);

  if (impl_->server) {
    impl_->server->forceStop();
  }
}
}  // namespace wrap
