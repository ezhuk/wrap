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

  wrap::App app(port, 1);
  app.get("/", []() -> folly::coro::Task<std::string> { co_return "Hello, world!\n"; });

  std::exception_ptr serverError;
  std::thread serverThread([&] {
    try {
      app.run();
    } catch (...) {
      serverError = std::current_exception();
    }
  });

  httplib::Client client(host, port);
  httplib::Result response = client.Get("/");
  for (int attempt = 0; attempt < 100 && !response; ++attempt) {
    std::this_thread::sleep_for(10ms);
    response = client.Get("/");
  }

  app.stop();
  serverThread.join();

  EXPECT_EQ(serverError, nullptr);
  ASSERT_TRUE(response);
  EXPECT_EQ(response->status, 200);
  EXPECT_EQ(response->body, "Hello, world!\n");
}
}  // namespace
