#include <benchmark/benchmark.h>

#include <random>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>

#include "config.hpp"

#include "limiters.hpp"

namespace {

  constexpr size_t kLatencyCapacity = 10000;

} // namespace

template <class LimiterType>
static void BM_Uniform_Traffic(benchmark::State& state) {

  double target_rate = static_cast<double>(state.range(0));
  double per_thread_rate = target_rate / static_cast<double>(state.threads());

  thread_local std::mt19937 gen(std::random_device{}());
  thread_local std::exponential_distribution<double> dist(1.0);
  dist.param(std::exponential_distribution<double>::param_type(per_thread_rate));

  std::vector<avito_limiter::key_type> keys = {"user_uniform"};
  static LimiterType limiter{keys.begin(), keys.end()};

  std::vector<double> latencies;
  latencies.reserve(kLatencyCapacity);

  for (auto _ : state) {

    double delay_sec = dist(gen);
    std::this_thread::sleep_for(std::chrono::duration<double>(delay_sec));

    auto start = std::chrono::high_resolution_clock::now();

    bool allowed = limiter.Access(keys[0]);
    benchmark::DoNotOptimize(allowed);

    auto end = std::chrono::high_resolution_clock::now();

    double duration_ns = std::chrono::duration<double, std::nano>(end - start).count();
    latencies.push_back(duration_ns);
  }

  if (!latencies.empty()) {
    std::sort(latencies.begin(), latencies.end());
    double p50 = latencies[static_cast<std::size_t>(latencies.size() * 0.50)];
    double p99 = latencies[static_cast<std::size_t>(latencies.size() * 0.99)];

    state.counters["p50_ns"] = benchmark::Counter(p50, benchmark::Counter::kAvgThreads); 
    state.counters["p99_ns"] = benchmark::Counter(p99, benchmark::Counter::kAvgThreads);
  }

  state.SetItemsProcessed(state.iterations());
}

using MyShardedWinLimiter = avito_limiter::ShardedWinLimiter<3U, true>;

BENCHMARK_TEMPLATE(BM_Uniform_Traffic, avito_limiter::MutexTokenLimiter)
    ->RangeMultiplier(2)
    ->Range(10, config::kTargetRatePerSec) 
    ->Threads(config::kNumThreads) 
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_Uniform_Traffic, avito_limiter::MutexWinLimiter)
    ->RangeMultiplier(2)
    ->Range(10, config::kTargetRatePerSec) 
    ->Threads(config::kNumThreads) 
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_Uniform_Traffic, MyShardedWinLimiter)
    ->RangeMultiplier(2)
    ->Range(10, config::kTargetRatePerSec) 
    ->Threads(config::kNumThreads) 
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);