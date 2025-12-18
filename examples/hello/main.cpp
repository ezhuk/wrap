#include <fmt/base.h>

#include "wrap/app.h"

namespace {
class User final : public wrap::Model<User> {
public:
  std::string email;
  std::optional<std::string> name;

  static folly::dynamic const& schema() {
    static folly::dynamic const schema = folly::parseJson(R"({
      "type": "object",
      "required": ["email"],
      "properties": {
        "email": { "type": "string" },
        "name": { "type": "string" }
      }
      })");
    return schema;
  }

  static std::optional<User> make(folly::dynamic const& data) {
    try {
      User user;
      user.email = data["email"].asString();
      if (data.count("name")) {
        user.name = data["name"].asString();
      }
      return user;
    } catch (...) {
      return std::nullopt;
    }
  }

  folly::dynamic dump() const {
    folly::dynamic data = folly::dynamic::object;
    data["email"] = email;
    if (name) {
      data["name"] = *name;
    }
    return data;
  }
};
}  // namespace

using namespace wrap;

int main(int argc, char** argv) {
  App app;

  app.post<User>("/users", [](User const& user) { fmt::print("_id={}\n", user._id()); });

  app.get("/users/:id", [](Request const& req, Response& res) {
    fmt::print("id={}\n", req.getParam("id"));
    res.status(200, "OK").body(R"({})");
  });

  app.run("0.0.0.0", 8080);
  return 0;
}
