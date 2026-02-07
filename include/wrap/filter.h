#pragma once

#include <proxygen/httpserver/Filters.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>

#include <memory>

namespace wrap::filter {
class TraceFilter final : public proxygen::Filter {
public:
  TraceFilter(proxygen::RequestHandler* downstream, std::string prefix)
      : proxygen::Filter(downstream), prefix_(std::move(prefix)) {}

  void sendHeaders(proxygen::HTTPMessage& msg) noexcept override {
    msg.getHeaders().set(
        "X-Request-Id", prefix_ + std::to_string(counter_.fetch_add(1, std::memory_order_relaxed))
    );
    proxygen::Filter::sendHeaders(msg);
  }

private:
  std::string prefix_;
  inline static std::atomic<std::uint64_t> counter_{1};
};

class TraceFilterFactory final : public proxygen::RequestHandlerFactory {
public:
  explicit TraceFilterFactory(std::string prefix = {}) : prefix_(std::move(prefix)) {}

  void onServerStart(folly::EventBase*) noexcept override {}
  void onServerStop() noexcept override {}

  proxygen::RequestHandler* onRequest(
      proxygen::RequestHandler* h, proxygen::HTTPMessage*
  ) noexcept override {
    return new TraceFilter(h, prefix_);
  }

private:
  std::string prefix_;
};

inline std::unique_ptr<proxygen::RequestHandlerFactory> trace(std::string prefix = {}) {
  return std::make_unique<wrap::filter::TraceFilterFactory>(std::move(prefix));
}
}  // namespace wrap::filter
