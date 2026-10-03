#include <folly/coro/Task.h>
#include <gtest/gtest.h>
#include <httplib.h>
#include <wrap/wrap.h>

#include <exception>
#include <memory>
#include <thread>

namespace {
class WrapTest : public ::testing::Test {
protected:
  static constexpr char host[] = "127.0.0.1";
  static constexpr int port = 18081;

  void SetUp() override {
    app_ = std::make_unique<wrap::App>(port, 1);

    app_->get("/", []() -> folly::coro::Task<std::string> { co_return "Hello, world!\n"; });

    serverThread_ = std::thread([this] {
      try {
        app_->run();
      } catch (...) {
        serverError_ = std::current_exception();
      }
    });
  }

  void TearDown() override {
    if (app_) {
      app_->stop();
    }

    if (serverThread_.joinable()) {
      serverThread_.join();
    }
  }

  httplib::Result get(std::string const& path) {
    httplib::Client client(host, port);
    return client.Get(path);
  }

  std::unique_ptr<wrap::App> app_;
  std::thread serverThread_;
  std::exception_ptr serverError_;
};

TEST_F(WrapTest, GetRoot) {
  auto response = get("/");

  EXPECT_EQ(serverError_, nullptr);

  ASSERT_TRUE(response);
  EXPECT_EQ(response->status, 200);
  EXPECT_EQ(response->body, "Hello, world!\n");
}
}  // namespace
