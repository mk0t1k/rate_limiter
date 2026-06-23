#include <benchmark/benchmark.h>

#include <random>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

#include "config.hpp"

#include "algorithms/limiters.hpp"

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
static void BM_Diurnal_Traffic(benchmark::State& state) {

  double base_rate = static_cast<double>(state.range(0));

  double amplitude = base_rate * 0.8;
  double period_sec = 10.0;

  thread_local std::mt19937 gen(std::random_device{}());
  thread_local std::exponential_distribution<double> dist(1.0);

  std::vector<avito_limiter::key_type> keys = {"user_diurnal"};
  static LimiterType limiter{keys.begin(), keys.end()};

  std::vector<double> latencies;
  latencies.reserve(kLatencyCapacity);

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

  if (!latencies.empty()) {
    std::sort(latencies.begin(), latencies.end());
    double p50 = latencies[static_cast<std::size_t>(latencies.size() * 0.50)];
    double p99 = latencies[static_cast<std::size_t>(latencies.size() * 0.99)];

    state.counters["p50_ns"] = benchmark::Counter(p50, benchmark::Counter::kAvgThreads);
    state.counters["p99_ns"] = benchmark::Counter(p99, benchmark::Counter::kAvgThreads);
  }
}

template <class LimiterType>
static void BM_Diurnal_CrossShard_Traffic(benchmark::State& state) {

  double base_rate = static_cast<double>(state.range(0));

  double amplitude = base_rate * 0.8;
  double period_sec = 10.0;

  thread_local std::mt19937 gen(std::random_device{}());
  thread_local std::exponential_distribution<double> dist(1.0);

  static const std::vector<avito_limiter::key_type> keys = CrossShardKeys();
  static LimiterType limiter{
      keys.begin(), keys.end(),
      std::tuple<std::size_t, float>{
          static_cast<std::size_t>(config::kBurstCapacity), 1.0F}};

  const avito_limiter::key_type& key =
    CrossShardKeyForThread(keys, static_cast<int>(state.thread_index()));

  std::vector<double> latencies;
  latencies.reserve(kLatencyCapacity);

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

  if (!latencies.empty()) {
    std::sort(latencies.begin(), latencies.end());
    double p50 = latencies[static_cast<std::size_t>(latencies.size() * 0.50)];
    double p99 = latencies[static_cast<std::size_t>(latencies.size() * 0.99)];

    state.counters["p50_ns"] = benchmark::Counter(p50, benchmark::Counter::kAvgThreads);
    state.counters["p99_ns"] = benchmark::Counter(p99, benchmark::Counter::kAvgThreads);
  }
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
