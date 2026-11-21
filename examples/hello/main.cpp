#include <gflags/gflags.h>
#include <wrap/wrap.h>

#include <string>

DEFINE_string(host, "0.0.0.0", "Hostname/IP to bind to");
DEFINE_int32(port, 8080, "Port to listen on");

int main(int argc, char** argv) {
  wrap::App app({
      .threads = 4,
  });

  app.get("/hello", [] { return "Hello world!\n"; });

  app.run(FLAGS_host, FLAGS_port);
}
