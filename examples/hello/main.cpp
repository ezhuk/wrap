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

  static std::optional<User> make(folly::dynamic const& data) {
    try {
      User user;
      user.id = data["id"].asString();
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
    data["id"] = id;
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
