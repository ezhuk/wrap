#include <fmt/base.h>

#include "wrap/app.h"
#include "wrap/filter.h"
#include "wrap/middleware.h"

using namespace wrap;

int main(int argc, char** argv) {
  App app;

  app.use(filter::trace("api-"));
  app.use(middleware::logger());

  app.get("/", []() { return R"({"message":"Hello, world!"})"; });

  app.run("0.0.0.0", 8080);
  return 0;
}
