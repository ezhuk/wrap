#include <fmt/base.h>

#include "wrap/app.h"

namespace {
static auto const kUserValidator = folly::jsonschema::makeValidator(folly::parseJson(R"JSON(
{
  "type": "object",
  "required": ["id"],
  "properties": {
    "id": { "type": "string" },
    "name": { "type": "string" }
  },
  "additionalProperties": false
}
)JSON"));
}  // namespace

using namespace wrap;

int main(int argc, char** argv) {
  App app;

  app.post("/users", [](Request const& req, Response& res) {
    auto const doc = req.json();
    if (kUserValidator->try_validate(doc)) {
      res.status(422, "Unprocessable Entity").body(R"({"error":"Schema validation failed"})");
    } else {
      res.status(201, "Created")
          .header("Location", fmt::format("/users/{}", doc["id"].asString()))
          .body(R"({})");
    }
  });

  app.get("/users/:id", [](Request const& req, Response& res) {
    fmt::print("id={}\n", req.getParam("id"));
    res.status(200, "OK").body(R"({})");
  });

  app.run("0.0.0.0", 8080);
  return 0;
}
