#include <benchmark/benchmark.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "common.hpp"
#include "config.hpp"

#include "algorithms/limiters.hpp"

template <class LimiterType>
static void BM_Burst_Traffic(benchmark::State& state) {

  std::vector<avito_limiter::key_type> keys = {"user_burst"};
  static LimiterType limiter{
      keys.begin(), keys.end(),
      std::tuple{config::kBurstCapacity, 1.0F}};

  static std::mutex s_mtx;
  static std::vector<std::vector<double>> s_per_thread_lats;
  static std::atomic<int> s_done{0};
  bench::BeginLatencyCollection(state, s_mtx, s_per_thread_lats, s_done);

  std::vector<double> latencies;
  latencies.reserve(bench::kLatencyCapacity);

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

  bench::FinishLatencyCollection(state, s_mtx, s_per_thread_lats, s_done,
                                 std::move(latencies));

  state.SetItemsProcessed(state.iterations());
}

template <class LimiterType>
static void BM_Burst_CrossShard_Traffic(benchmark::State& state) {

  static const std::vector<avito_limiter::key_type> keys = bench::CrossShardKeys();
  static LimiterType limiter{
      keys.begin(), keys.end(),
      std::tuple{config::kBurstCapacity, 1.0F}};

  const avito_limiter::key_type& key =
      bench::CrossShardKeyForThread(keys, static_cast<int>(state.thread_index()));

  static std::mutex s_mtx_cs;
  static std::vector<std::vector<double>> s_per_thread_lats_cs;
  static std::atomic<int> s_done_cs{0};
  bench::BeginLatencyCollection(state, s_mtx_cs, s_per_thread_lats_cs, s_done_cs);

  std::vector<double> latencies;
  latencies.reserve(bench::kLatencyCapacity);

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

  bench::FinishLatencyCollection(state, s_mtx_cs, s_per_thread_lats_cs, s_done_cs,
                                 std::move(latencies));

  state.SetItemsProcessed(state.iterations());
}

using ShardedWinLimiterAligned =
    avito_limiter::ShardedWinLimiter<config::kShardCapacity, true>;
using ShardedWinLimiterCompact =
    avito_limiter::ShardedWinLimiter<config::kShardCapacity, false>;

BENCHMARK_TEMPLATE(BM_Burst_Traffic, avito_limiter::MutexTokenLimiter)
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_Burst_Traffic, avito_limiter::MutexWinLimiter)
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_Burst_CrossShard_Traffic, ShardedWinLimiterAligned)
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_Burst_CrossShard_Traffic, ShardedWinLimiterCompact)
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);
