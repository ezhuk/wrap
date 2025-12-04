#include <fmt/base.h>

#include "wrap/app.h"

using namespace wrap;

int main(int argc, char** argv) {
  App app;

  app.get("/users/:id", [](Request const& req, Response& res) {
    fmt::print("id={}\n", req.getParam("id"));
    res.status(200, "OK").body(R"({})");
  });

  app.run("0.0.0.0", 8080);
  return 0;
}
