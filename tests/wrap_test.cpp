#include <gtest/gtest.h>
#include <httplib.h>

#include <chrono>
#include <memory>
#include <thread>

#include "wrap/app.h"

using namespace wrap;

class WrapTest : public testing::Test {
protected:
  static constexpr char const* host = "127.0.0.1";
  static constexpr int port = 8081;

  void SetUp() override {
    app_ = std::make_unique<App>();
    thread_ = std::thread([&] { app_->run(host, port); });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    client_ = std::make_unique<httplib::Client>(host, port);
  }

  void TearDown() override {
    client_.reset();
    app_->stop();
    thread_.join();
  }

  std::unique_ptr<App> app_;
  std::thread thread_;
  std::unique_ptr<httplib::Client> client_;
};

TEST_F(WrapTest, GetTest) {
  app_->get("/", []() { return "TEST"; });

  auto const res = client_->Get("/");
  EXPECT_EQ(res->status, 200);
  EXPECT_EQ(res->body, "TEST");
}
