#include <folly/coro/Task.h>
#include <gtest/gtest.h>
#include <httplib.h>
#include <wrap/wrap.h>

#include <chrono>
#include <exception>
#include <thread>

namespace {
using namespace std::chrono_literals;

TEST(WrapTest, GetRoot) {
  constexpr char host[] = "127.0.0.1";

  constexpr int port = 18081;

  wrap::App app({
      .host = host,
      .port = port,
      .threads = 1,
  });

  app.use(wrap::middleware::header("X-Wrap-Test", "1"));

  app.get("/", [](wrap::Request const&) -> folly::coro::Task<wrap::Response> {
    co_return wrap::Response::text("Hello, world!\n");
  });

  std::exception_ptr serverError;

  std::thread serverThread([&] {
    try {
      app.run();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  httplib::Client client(host, port);

  std::shared_ptr<httplib::Response> response;

  for (int attempt = 0; attempt < 100; ++attempt) {
    response = client.Get("/");

    if (response) {
      break;
    }

    std::this_thread::sleep_for(10ms);
  }

  app.stop();

  serverThread.join();

  EXPECT_EQ(serverError, nullptr);

  ASSERT_NE(response, nullptr);

  EXPECT_EQ(response->status, 200);

  EXPECT_EQ(response->body, "Hello, world!\n");

  EXPECT_EQ(response->get_header_value("X-Wrap-Test"), "1");
}
}  // namespace
