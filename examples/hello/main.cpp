#include <fmt/base.h>
#include <gflags/gflags.h>

#include "wrap/app.h"

using namespace wrap;

DEFINE_string(host, "0.0.0.0", "Host to listen on");
DEFINE_int32(port, 8080, "Port to listen on");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  App app;

  app.get("/", []() { return R"({"message":"Hello, world!"})"; });

  app.run(FLAGS_host, FLAGS_port);
  return 0;
}
