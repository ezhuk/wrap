#include <wrap/wrap.h>

#include <string>

int main() {
  wrap::App app(8080, 1);

  app.get("/foo", [] { return "Hello from /foo!\n"; });

  app.get("/bar", []() -> wrap::Task<std::string> {
    co_return "Hello asynchronously from /bar!\n";
  });

  app.run();
}
