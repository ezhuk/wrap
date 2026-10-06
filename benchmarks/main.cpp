#include <benchmark/benchmark.h>
#include <httplib.h>
#include <wrap/wrap.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>

namespace {
class WrapBenchmark : public benchmark::Fixture {
protected:
  static constexpr char host[] = "127.0.0.1";
  static constexpr int port = 18082;

  void SetUp(const benchmark::State& state) override {
    if (state.thread_index() != 0) {
      return;
    }

    app_ = std::make_unique<wrap::App>(port, static_cast<std::size_t>(state.range(0)));
    app_->get("/foo", [] { return "foo\n"; });
    app_->get("/bar", []() -> wrap::Task<std::string> { co_return "bar\n"; });

    thread_ = std::thread([] { app_->run(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  void TearDown(const benchmark::State& state) override {
    if (state.thread_index() != 0) {
      return;
    }

    app_->stop();
    if (thread_.joinable()) {
      thread_.join();
    }
    app_.reset();
  }

  inline static std::unique_ptr<wrap::App> app_;
  inline static std::thread thread_;
};

BENCHMARK_DEFINE_F(WrapBenchmark, SyncGet)(benchmark::State& state) {
  httplib::Client client(host, port);
  for (auto _ : state) {
    auto response = client.Get("/foo");
    benchmark::DoNotOptimize(response);
    if (!response || response->status != 200) {
      state.SkipWithError("GET /foo failed");
      break;
    }
  }
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK_DEFINE_F(WrapBenchmark, AsyncGet)(benchmark::State& state) {
  httplib::Client client(host, port);
  for (auto _ : state) {
    auto response = client.Get("/bar");
    benchmark::DoNotOptimize(response);
    if (!response || response->status != 200) {
      state.SkipWithError("GET /bar failed");
      break;
    }
  }
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(WrapBenchmark, SyncGet)
    ->RangeMultiplier(2)
    ->Range(1, 8)
    ->ThreadRange(1, 8)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_REGISTER_F(WrapBenchmark, AsyncGet)
    ->RangeMultiplier(2)
    ->Range(1, 8)
    ->ThreadRange(1, 8)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);
}  // namespace
