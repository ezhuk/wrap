#include <fmt/base.h>

#include "wrap/app.h"

int main(int argc, char** argv) {
  wrap::App app;

  app.get("/hello", [](wrap::Request const& req, wrap::Response& res) {
    fmt::print("{} {}\n", req.getMethod(), req.getURL());
    res.status(200, "OK").body("{}");
  });

  app.run("0.0.0.0", 8080);
  return 0;
}
