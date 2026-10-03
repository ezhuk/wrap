#include <folly/coro/Task.h>
#include <wrap/wrap.h>

#include <string>

int main() {
  wrap::App app(8080, 1);
  app.get("/", []() -> folly::coro::Task<std::string> { co_return "Hello, world!\n"; });
  app.run();
}
