#include <folly/coro/Task.h>
#include <wrap/wrap.h>

int main() {
  wrap::App app;

  app.use(wrap::middleware::header("Server", "wrap"));

  app.get("/", [](wrap::Request const&) -> folly::coro::Task<wrap::Response> {
    co_return wrap::Response::text("Hello, world!\n");
  });

  app.run();

  return 0;
}
