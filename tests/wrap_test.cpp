#include <gtest/gtest.h>
#include <httplib.h>

#include <chrono>
#include <thread>

#include "wrap/app.h"

TEST(WrapTest, Get) {
  wrap::App app;
  app.get("/", []() { return "TEST"; });
  std::thread thread([&] { app.run("127.0.0.1", 8081); });
  std::this_thread::sleep_for(std::chrono::seconds(1));

  httplib::Client client("127.0.0.1", 8081);
  auto const res = client.Get("/");
  EXPECT_EQ(res->status, 200);
  EXPECT_EQ(res->body, "TEST");

  app.stop();
  thread.join();
}
