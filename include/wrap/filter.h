#pragma once

#include <proxygen/httpserver/Filters.h>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>

#include <memory>
#include <string>
#include <tuple>

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

template <class T, class... U>
class FilterFactory final : public proxygen::RequestHandlerFactory {
public:
  explicit FilterFactory(U... args) : args_(std::move(args)...) {}

  void onServerStart(folly::EventBase*) noexcept override {}

  void onServerStop() noexcept override {}

  proxygen::RequestHandler* onRequest(
      proxygen::RequestHandler* h, proxygen::HTTPMessage*
  ) noexcept override {
    return std::apply(
        [&](auto&... xs) -> proxygen::RequestHandler* { return new T(h, xs...); }, args_
    );
  }

private:
  std::tuple<U...> args_;
};

inline std::unique_ptr<proxygen::RequestHandlerFactory> trace(std::string prefix = {}) {
  return std::make_unique<FilterFactory<TraceFilter, std::string>>(std::move(prefix));
}
}  // namespace wrap::filter
