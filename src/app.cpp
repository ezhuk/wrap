#include "wrap/app.h"

#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/coro/HTTPCoroSession.h>
#include <proxygen/lib/http/coro/HTTPEvents.h>
#include <proxygen/lib/http/coro/HTTPFixedSource.h>
#include <proxygen/lib/http/coro/server/HTTPServer.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace wrap {

class App::Handler final : public proxygen::coro::HTTPHandler {
public:
  void addRoute(
      proxygen::HTTPMethod method, std::string path, GetHandler getHandler,
      PostHandler postHandler = {}
  ) {
    for (auto const& route : routes_) {
      if (route.method == method && route.path == path) {
        throw std::invalid_argument("duplicate route: " + path);
      }
    }

    routes_.push_back({
        .method = method,
        .path = std::move(path),
        .getHandler = std::move(getHandler),
        .postHandler = std::move(postHandler),
    });
  }

  folly::coro::Task<proxygen::coro::HTTPSourceHolder> handleRequest(
      folly::EventBase*, proxygen::coro::HTTPSessionContextPtr,
      proxygen::coro::HTTPSourceHolder request
  ) override {
    auto event = co_await request.readHeaderEvent();
    auto method = event.headers->getMethod();
    auto path = event.headers->getPathAsStringPiece();

    for (auto const& route : routes_) {
      if (method != route.method || path != route.path) {
        continue;
      }

      if (method == proxygen::HTTPMethod::GET) {
        if (!event.eom) {
          request.stopReading();
        }

        auto body = co_await route.getHandler();

        co_return proxygen::coro::HTTPFixedSource::makeFixedResponse(200, std::move(body));
      }

      if (method == proxygen::HTTPMethod::POST) {
        std::string body;

        if (!event.eom) {
          constexpr std::size_t maxBodySize = 1024 * 1024;

          bool eom = false;

          while (!eom) {
            auto chunk = co_await request.readBodyEvent();
            eom = chunk.eom;

            if (chunk.eventType == proxygen::coro::HTTPBodyEvent::SUSPEND) {
              co_await std::move(chunk.event.resume);
              continue;
            }

            if (chunk.eventType != proxygen::coro::HTTPBodyEvent::BODY) {
              continue;
            }

            auto& buffers = chunk.event.body;
            auto size = buffers.chainLength();

            if (size > maxBodySize - body.size()) {
              if (!eom) {
                request.stopReading();
              }

              co_return proxygen::coro::HTTPFixedSource::makeFixedResponse(
                  413, "Payload Too Large\n"
              );
            }

            auto data = buffers.move();

            if (data) {
              for (auto const& buffer : *data) {
                body.append(reinterpret_cast<const char*>(buffer.data()), buffer.length());
              }
            }
          }
        }

        auto response = co_await route.postHandler(std::move(body));

        co_return proxygen::coro::HTTPFixedSource::makeFixedResponse(200, std::move(response));
      }
    }

    if (!event.eom) {
      request.stopReading();
    }

    co_return proxygen::coro::HTTPFixedSource::makeFixedResponse(404, "Not Found\n");
  }

private:
  struct Route {
    proxygen::HTTPMethod method;
    std::string path;
    GetHandler getHandler;
    PostHandler postHandler;
  };

  std::vector<Route> routes_;
};

App::App(AppOptions options)
    : options_(std::move(options)), handler_(std::make_shared<Handler>()) {}

App::~App() { stop(); }

App& App::get(std::string path, GetHandler handler) {
  handler_->addRoute(proxygen::HTTPMethod::GET, std::move(path), std::move(handler));
  return *this;
}

App& App::post(std::string path, PostHandler handler) {
  handler_->addRoute(proxygen::HTTPMethod::POST, std::move(path), {}, std::move(handler));
  return *this;
}

void App::run(std::string host, std::uint16_t port) {
  proxygen::coro::HTTPServer::Config config;
  config.socketConfig.bindAddress.setFromIpPort(host, port);
  config.numIOThreads = options_.threads;
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
