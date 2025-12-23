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

  app.put<User>("/users/:id", [](std::string const& id, User const& user) {
    fmt::print("id={}\n", id);
  });

  app.get<User>("/users/{id}", [](std::string const& id) {
    fmt::print("id={}\n", id);
    User user;
    user.email = "user@example.com";
    return user;
  });

  app.get<std::vector<User>>("/users", []() {
    User user1;
    user1.email = "user1@example.com";
    User user2;
    user2.email = "user2@example.com";
    return std::vector<User>{user1, user2};
  });

  app.run("0.0.0.0", 8080);
  return 0;
}
