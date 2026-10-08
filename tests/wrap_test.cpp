#include <gtest/gtest.h>
#include <httplib.h>
#include <wrap/wrap.h>

#include <exception>
#include <memory>
#include <string>
#include <thread>

namespace {
class WrapTest : public ::testing::Test {
protected:
  static constexpr char host[] = "127.0.0.1";
  static constexpr int port = 18081;

  static void SetUpTestSuite() {
    app_ = std::make_unique<wrap::App>();
    app_->get("/foo", [] { return "foo\n"; });
    app_->get("/bar", []() -> wrap::Task<std::string> { co_return "bar\n"; });

    thread_ = std::thread([] {
      try {
        app_->run(host, port);
      } catch (...) {
        error_ = std::current_exception();
      }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  static void TearDownTestSuite() {
    app_->stop();
    if (thread_.joinable()) {
      thread_.join();
    }
    app_.reset();
  }

  httplib::Result get(std::string const& path) {
    httplib::Client client(host, port);
    return client.Get(path);
  }

  inline static std::unique_ptr<wrap::App> app_;
  inline static std::thread thread_;
  inline static std::exception_ptr error_;
};

TEST_F(WrapTest, GetRoot) {
  auto response = get("/");

  EXPECT_EQ(error_, nullptr);
  ASSERT_TRUE(response);
  EXPECT_EQ(response->status, 404);
  EXPECT_EQ(response->body, "Not Found\n");
}

TEST_F(WrapTest, GetSyncRoute) {
  auto response = get("/foo");

  EXPECT_EQ(error_, nullptr);
  ASSERT_TRUE(response);
  EXPECT_EQ(response->status, 200);
  EXPECT_EQ(response->body, "foo\n");
}

TEST_F(WrapTest, GetAsyncRoute) {
  auto response = get("/bar");

  EXPECT_EQ(error_, nullptr);
  ASSERT_TRUE(response);
  EXPECT_EQ(response->status, 200);
  EXPECT_EQ(response->body, "bar\n");
}
}  // namespace
