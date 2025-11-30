#include <fmt/base.h>

#include "wrap/app.h"

int main(int argc, char** argv) {
  wrap::App app;

  app.get("/hello", [](proxygen::HTTPMessage const& req, proxygen::ResponseBuilder& res) {
    fmt::print("{} {}\n", req.getMethodString(), req.getURL());
    res.status(200, "OK").body("{}").sendWithEOM();
  });

  app.run("0.0.0.0", 8080);
  return 0;
}
