#include <fmt/base.h>

#include "wrap/app.h"

namespace {
class User final : public wrap::Model<User> {
public:
  std::string id;
  std::optional<std::string> name;

  static folly::dynamic const& schema() {
    static folly::dynamic const schema =
        folly::dynamic::object("type", "object")("required", folly::dynamic::array("id"))(
            "properties", folly::dynamic::object("id", folly::dynamic::object("type", "string"))(
                              "name", folly::dynamic::object("type", "string")
                          )
        );
    return schema;
  }
};
}  // namespace

namespace folly {
template <>
struct DynamicConverter<User> {
  static User convert(folly::dynamic const& data) {
    User user;
    user.id = data["id"].asString();
    if (data.count("name")) {
      user.name = data["name"].asString();
    }
    return user;
  };
};
}  // namespace folly

using namespace wrap;

int main(int argc, char** argv) {
  App app;

  app.post("/users", [](Request const& req, Response& res) {
    auto const user = User::parse(req.body());
    if (!user) {
      res.status(422, "Unprocessable Entity").body(R"({"error":"Could not parse user data"})");
      return;
    }

    res.status(201, "Created").header("Location", fmt::format("/users/{}", user->id)).body(R"({})");
  });

  app.get("/users/:id", [](Request const& req, Response& res) {
    fmt::print("id={}\n", req.getParam("id"));
    res.status(200, "OK").body(R"({})");
  });

  app.run("0.0.0.0", 8080);
  return 0;
}
