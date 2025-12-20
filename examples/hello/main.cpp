#include <fmt/base.h>

#include "wrap/app.h"

namespace {
class User final : public wrap::Model<User> {
public:
  std::string email;
  std::optional<std::string> name;

  static constexpr auto meta() {
    return std::make_tuple(wrap::field("email", &User::email), wrap::field("name", &User::name));
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
