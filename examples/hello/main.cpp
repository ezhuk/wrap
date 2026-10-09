#include <wrap/wrap.h>

#include <string>

int main() {
  wrap::App app({
      .threads = 4,
  });

  app.get("/foo", [] { return "Hello from /foo!\n"; });

  app.get("/bar", []() -> wrap::Task<std::string> {
    co_return "Hello asynchronously from /bar!\n";
  });

  app.run("0.0.0.0", 8080);
}
