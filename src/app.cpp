#include "wrap/app.h"

#include <folly/SocketAddress.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/coro/HTTPCoroSession.h>
#include <proxygen/lib/http/coro/HTTPFixedSource.h>
#include <proxygen/lib/http/coro/server/HTTPServer.h>

#include <stdexcept>
#include <utility>

namespace wrap {
class App::Handler final : public proxygen::coro::HTTPHandler {
public:
  explicit Handler(GetHandler handler) : handler_(std::move(handler)) {}

  folly::coro::Task<proxygen::coro::HTTPSourceHolder> handleRequest(
      folly::EventBase*, proxygen::coro::HTTPSessionContextPtr,
      proxygen::coro::HTTPSourceHolder request
  ) override {
    auto event = co_await request.readHeaderEvent();

    auto& message = *event.headers;

    if (!event.eom) {
      request.stopReading();
    }

    if (message.getMethod() != proxygen::HTTPMethod::GET || message.getPathAsStringPiece() != "/") {
      co_return proxygen::coro::HTTPFixedSource::makeFixedResponse(404, "Not Found\n");
    }

    auto body = co_await handler_();

    co_return proxygen::coro::HTTPFixedSource::makeFixedResponse(200, std::move(body));
  }

private:
  GetHandler handler_;
};

App::App(std::uint16_t port, std::size_t threads) : port_(port), threads_(threads) {}

App::~App() { stop(); }

App& App::get(std::string path, GetHandler handler) {
  if (path != "/") {
    throw std::invalid_argument("only GET / is currently supported");
  }

  handler_ = std::make_shared<Handler>(std::move(handler));

  return *this;
}

void App::run() {
  if (!handler_) {
    throw std::logic_error("no GET / handler registered");
  }

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
