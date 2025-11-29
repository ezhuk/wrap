#include "wrap/app.h"

#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>
#include <proxygen/httpserver/filters/DirectResponseHandler.h>

#include <memory>
#include <thread>
#include <unordered_map>

namespace wrap {
namespace {
class HandlerFactory final : public proxygen::RequestHandlerFactory {
public:
  void onServerStart(folly::EventBase*) noexcept override {}

  void onServerStop() noexcept override {}

  proxygen::RequestHandler* onRequest(
      proxygen::RequestHandler* h, proxygen::HTTPMessage* msg
  ) noexcept override {
    return new proxygen::DirectResponseHandler(404, "Not Found", "{\"error\":\"Not Found\"}");
  }
};

std::unique_ptr<proxygen::HTTPServer> server;
std::thread thread;
}  // namespace

App::App(AppOptions options) : options_(std::move(options)) {
  proxygen::HTTPServerOptions opts;
  opts.threads = options_.threads;
  opts.handlerFactories = proxygen::RequestHandlerChain().addThen<HandlerFactory>().build();
  server = std::make_unique<proxygen::HTTPServer>(std::move(opts));
  server->bind(
      {{folly::SocketAddress(options_.host, options_.port, true),
        proxygen::HTTPServer::Protocol::HTTP}}
  );
  thread = std::thread([]() { server->start(); });
}

App::~App() {
  if (server) {
    server->stop();
    server.reset();
  }
  if (thread.joinable()) {
    thread.join();
  }
}
}  // namespace wrap
