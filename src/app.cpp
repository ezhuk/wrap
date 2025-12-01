#include "wrap/app.h"

#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>
#include <proxygen/httpserver/filters/DirectResponseHandler.h>

namespace wrap {
namespace {
class RequestHandler final : public proxygen::RequestHandler {
public:
  explicit RequestHandler(std::unordered_map<std::string, App::Handler>* handlers)
      : handlers_(handlers) {}

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
    auto handler = getHandler(request_->getMethodString() + ":" + request_->getURL());
    if (handler) {
      proxygen::ResponseBuilder response(downstream_);
      handler(Request(request_.get()), response);
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
  App::Handler getHandler(std::string const& key) const {
    auto iter = handlers_->find(key);
    if (handlers_->end() != iter) {
      return iter->second;
    }
    return nullptr;
  }

private:
  std::unordered_map<std::string, App::Handler>* handlers_;
  std::unique_ptr<proxygen::HTTPMessage> request_;
  std::unique_ptr<folly::IOBuf> body_;
};

class HandlerFactory final : public proxygen::RequestHandlerFactory {
public:
  explicit HandlerFactory(std::unordered_map<std::string, App::Handler>* handlers)
      : handlers_(handlers) {}

  void onServerStart(folly::EventBase*) noexcept override {}

  void onServerStop() noexcept override {}

  proxygen::RequestHandler* onRequest(
      proxygen::RequestHandler*, proxygen::HTTPMessage*
  ) noexcept override {
    return new RequestHandler(handlers_);
  }

private:
  std::unordered_map<std::string, App::Handler>* handlers_;
};
}  // namespace

App::App(AppOptions options) : options_(std::move(options)) {}

App& App::get(std::string const& path, Handler handler) {
  handlers_["GET:" + path] = std::move(handler);
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
      proxygen::RequestHandlerChain().addThen<HandlerFactory>(&handlers_).build();
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
