#include <fmt/base.h>

#include "wrap/app.h"

int main(int argc, char** argv) {
  wrap::App app;

  app.get("/hello", [](wrap::Request const& req, proxygen::ResponseBuilder& res) {
    fmt::print("{} {}\n", req.getMethod(), req.getURL());
    res.status(200, "OK").body("{}").sendWithEOM();
  });

  app.run("0.0.0.0", 8080);
  return 0;
}
