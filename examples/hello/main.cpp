#include <fmt/base.h>

#include "wrap/app.h"

using namespace wrap;

int main(int argc, char** argv) {
  App app;

  app.get("/", []() { return R"({"message":"Hello, world!"})"; });

  app.run("0.0.0.0", 8080);
  return 0;
}
