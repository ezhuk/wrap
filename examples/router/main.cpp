#include <fmt/base.h>

#include "wrap/app.h"
#include "wrap/router.h"

using namespace wrap;

int main(int argc, char** argv) {
  App app;

  Router users(app, "/users");
  users.get("/", []() { return R"({"users":[]})"; });
  users.get("/{id:int}", [](int id) { return fmt::format(R"({{"id":{}}})", id); });

  app.run("0.0.0.0", 8080);
  return 0;
}
