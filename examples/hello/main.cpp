#include <fmt/base.h>

#include "wrap/app.h"

using namespace wrap;

int main(int argc, char** argv) {
  App app;

  app.post("/users", [](Request const& req, Response& res) {
    auto const doc = req.json();
    if (doc.count("id")) {
      res.status(201, "Created")
          .header("Location", fmt::format("/users/{}", doc["id"].asString()))
          .body(R"({})");
    } else {
      res.status(400, "Bad Request").body(R"({"error":"User id is not found"})");
    }
  });

  app.get("/users/:id", [](Request const& req, Response& res) {
    fmt::print("id={}\n", req.getParam("id"));
    res.status(200, "OK").body(R"({})");
  });

  app.run("0.0.0.0", 8080);
  return 0;
}
