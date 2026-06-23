#include <benchmark/benchmark.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <mutex>
#include <numbers>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "common.hpp"
#include "config.hpp"

#include "algorithms/limiters.hpp"

template <class LimiterType>
static void BM_Diurnal_Traffic(benchmark::State& state) {

  double base_rate = static_cast<double>(state.range(0));

  double amplitude = base_rate * 0.8;
  double period_sec = 10.0;

  thread_local std::mt19937 gen(std::random_device{}());
  thread_local std::exponential_distribution<double> dist(1.0);

  std::vector<avito_limiter::key_type> keys = {"user_diurnal"};
  static LimiterType limiter{
      keys.begin(), keys.end(),
      std::tuple{config::kBurstCapacity, 1.0F}};

  static std::mutex s_mtx;
  static std::vector<std::vector<double>> s_per_thread_lats;
  static std::atomic<int> s_done{0};
  bench::BeginLatencyCollection(state, s_mtx, s_per_thread_lats, s_done);

  std::vector<double> latencies;
  latencies.reserve(bench::kLatencyCapacity);

  auto start_time = std::chrono::steady_clock::now();

  for (auto _ : state) {

    auto now = std::chrono::steady_clock::now();
    double t = std::chrono::duration<double>(now - start_time).count();

    double current_rate = base_rate + amplitude * std::sin(2 * std::numbers::pi * t / period_sec);
    current_rate = std::max(current_rate, 1.0);

    double per_thread_rate = current_rate / static_cast<double>(state.threads());
    dist.param(std::exponential_distribution<double>::param_type(per_thread_rate));

    double delay_sec = dist(gen);
    std::this_thread::sleep_for(std::chrono::duration<double>(delay_sec));

    auto req_start = std::chrono::high_resolution_clock::now();
        
    bool allowed = limiter.Access(keys[0]);
    benchmark::DoNotOptimize(allowed); 
        
    auto req_end = std::chrono::high_resolution_clock::now();

    double duration_ns = std::chrono::duration<double, std::nano>(req_end - req_start).count();
    latencies.push_back(duration_ns);
  }

  bench::FinishLatencyCollection(state, s_mtx, s_per_thread_lats, s_done,
                                 std::move(latencies));

  state.SetItemsProcessed(state.iterations());
  bench::PublishKeyCount(state, limiter);
}

template <class LimiterType>
static void BM_Diurnal_CrossShard_Traffic(benchmark::State& state) {

  double base_rate = static_cast<double>(state.range(0));

  double amplitude = base_rate * 0.8;
  double period_sec = 10.0;

  thread_local std::mt19937 gen(std::random_device{}());
  thread_local std::exponential_distribution<double> dist(1.0);

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

  auto start_time = std::chrono::steady_clock::now();

  for (auto _ : state) {

    auto now = std::chrono::steady_clock::now();
    double t = std::chrono::duration<double>(now - start_time).count();

    double current_rate = base_rate + amplitude * std::sin(2 * std::numbers::pi * t / period_sec);
    current_rate = std::max(current_rate, 1.0);

    double per_thread_rate = current_rate / static_cast<double>(state.threads());
    dist.param(std::exponential_distribution<double>::param_type(per_thread_rate));

    double delay_sec = dist(gen);
    std::this_thread::sleep_for(std::chrono::duration<double>(delay_sec));

    auto req_start = std::chrono::high_resolution_clock::now();

    bool allowed = limiter.Access(key);
    benchmark::DoNotOptimize(allowed);

    auto req_end = std::chrono::high_resolution_clock::now();

    double duration_ns = std::chrono::duration<double, std::nano>(req_end - req_start).count();
    latencies.push_back(duration_ns);
  }

  bench::FinishLatencyCollection(state, s_mtx_cs, s_per_thread_lats_cs, s_done_cs,
                                 std::move(latencies));

  state.SetItemsProcessed(state.iterations());
  bench::PublishKeyCount(state, limiter);
}

using ShardedWinLimiterAligned =
  avito_limiter::ShardedWinLimiter<config::kShardCapacity, true>;
using ShardedWinLimiterCompact =
  avito_limiter::ShardedWinLimiter<config::kShardCapacity, false>;

BENCHMARK_TEMPLATE(BM_Diurnal_Traffic, avito_limiter::MutexTokenLimiter)
    ->RangeMultiplier(2)
    ->Range(10, config::kTargetRatePerSec) 
    ->Threads(config::kNumThreads) 
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_Diurnal_Traffic, avito_limiter::MutexWinLimiter)
    ->RangeMultiplier(2)
    ->Range(10, config::kTargetRatePerSec) 
    ->Threads(config::kNumThreads) 
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_Diurnal_CrossShard_Traffic, ShardedWinLimiterAligned)
    ->RangeMultiplier(2)
    ->Range(10, config::kTargetRatePerSec)
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_Diurnal_CrossShard_Traffic, ShardedWinLimiterCompact)
    ->RangeMultiplier(2)
    ->Range(10, config::kTargetRatePerSec)
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);
