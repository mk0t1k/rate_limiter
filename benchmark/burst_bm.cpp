#include <benchmark/benchmark.h>

#include <random>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <string>

#include "config.hpp"

#include "limiters.hpp"

namespace {

  constexpr size_t kLatencyCapacity = 10000;

  std::vector<avito_limiter::key_type> CrossShardKeys() {
    std::vector<avito_limiter::key_type> keys;
    keys.reserve(config::kShardCapacity * 2);
    for (std::size_t i = 0; i < config::kShardCapacity * 2; ++i) {
      keys.push_back("k" + std::to_string(i));
    }
    return keys;
  }

  const avito_limiter::key_type& CrossShardKeyForThread(
      const std::vector<avito_limiter::key_type>& keys,
      int thread_index) {
    const std::size_t key_index =
        (thread_index % 2 == 0)
            ? config::kCrossShardKeyLastInFirstShard
            : config::kCrossShardKeyFirstInSecondShard;
    return keys[key_index];
  }

} // namespace

template <class LimiterType>
static void BM_Burst_Traffic(benchmark::State& state) {

  std::vector<avito_limiter::key_type> keys = {"user_burst"};
  static LimiterType limiter{keys.begin(), keys.end()};

  std::vector<double> latencies;
  latencies.reserve(kLatencyCapacity);

  int burst_counter = 0;

  for (auto _ : state) {

    if (burst_counter < config::kBurstSize) {
      std::this_thread::sleep_for(
          std::chrono::microseconds(config::kBurstInterReqUs));
    } else {
      std::this_thread::sleep_for(
          std::chrono::duration<double>(config::kBurstSleepSec));
      burst_counter = 0;
    }

    auto start = std::chrono::high_resolution_clock::now();

    bool allowed = limiter.Access(keys[0]);
    benchmark::DoNotOptimize(allowed);

    auto end = std::chrono::high_resolution_clock::now();

    double duration_ns =
        std::chrono::duration<double, std::nano>(end - start).count();
    latencies.push_back(duration_ns);

    ++burst_counter;
  }

  if (!latencies.empty()) {
    std::sort(latencies.begin(), latencies.end());
    double p50 = latencies[static_cast<std::size_t>(latencies.size() * 0.50)];
    double p99 = latencies[static_cast<std::size_t>(latencies.size() * 0.99)];

    state.counters["p50_ns"] =
        benchmark::Counter(p50, benchmark::Counter::kAvgThreads);
    state.counters["p99_ns"] =
        benchmark::Counter(p99, benchmark::Counter::kAvgThreads);
  }

  state.SetItemsProcessed(state.iterations());
}

template <class LimiterType>
static void BM_Burst_CrossShard_Traffic(benchmark::State& state) {

  static const std::vector<avito_limiter::key_type> keys = CrossShardKeys();
  static const avito_limiter::SlidingWindowAlgo init_alg{
      static_cast<std::size_t>(config::kBurstCapacity), 1.0F};
  static LimiterType limiter{keys.begin(), keys.end(), init_alg};

  const avito_limiter::key_type& key =
      CrossShardKeyForThread(keys, static_cast<int>(state.thread_index()));

  std::vector<double> latencies;
  latencies.reserve(kLatencyCapacity);

  int burst_counter = 0;

  for (auto _ : state) {

    if (burst_counter < config::kBurstSize) {
      std::this_thread::sleep_for(
          std::chrono::microseconds(config::kBurstInterReqUs));
    } else {
      std::this_thread::sleep_for(
          std::chrono::duration<double>(config::kBurstSleepSec));
      burst_counter = 0;
    }

    auto start = std::chrono::high_resolution_clock::now();

    bool allowed = limiter.Access(key);
    benchmark::DoNotOptimize(allowed);

    auto end = std::chrono::high_resolution_clock::now();

    double duration_ns =
        std::chrono::duration<double, std::nano>(end - start).count();
    latencies.push_back(duration_ns);

    ++burst_counter;
  }

  if (!latencies.empty()) {
    std::sort(latencies.begin(), latencies.end());
    double p50 = latencies[static_cast<std::size_t>(latencies.size() * 0.50)];
    double p99 = latencies[static_cast<std::size_t>(latencies.size() * 0.99)];

    state.counters["p50_ns"] =
        benchmark::Counter(p50, benchmark::Counter::kAvgThreads);
    state.counters["p99_ns"] =
        benchmark::Counter(p99, benchmark::Counter::kAvgThreads);
  }

  state.SetItemsProcessed(state.iterations());
}

using ShardedWinLimiterAligned =
    avito_limiter::ShardedWinLimiter<config::kShardCapacity, true>;
using ShardedWinLimiterCompact =
    avito_limiter::ShardedWinLimiter<config::kShardCapacity, false>;

BENCHMARK_TEMPLATE(BM_Burst_Traffic, avito_limiter::MutexTokenLimiter)
    ->RangeMultiplier(2)
    ->Range(10, config::kTargetRatePerSec)
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_Burst_Traffic, avito_limiter::MutexWinLimiter)
    ->RangeMultiplier(2)
    ->Range(10, config::kTargetRatePerSec)
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_Burst_CrossShard_Traffic, ShardedWinLimiterAligned)
    ->RangeMultiplier(2)
    ->Range(10, config::kTargetRatePerSec)
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_Burst_CrossShard_Traffic, ShardedWinLimiterCompact)
    ->RangeMultiplier(2)
    ->Range(10, config::kTargetRatePerSec)
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);
