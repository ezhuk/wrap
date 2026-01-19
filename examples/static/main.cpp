#include <fmt/base.h>

#include "wrap/app.h"
#include "wrap/middleware.h"
#include "wrap/static.h"

using namespace wrap;

int main(int argc, char** argv) {
  App app;

  app.use(middleware::logger());

  app.get("/", serve_static("./public"));

  app.run("0.0.0.0", 8080);
  return 0;
}
