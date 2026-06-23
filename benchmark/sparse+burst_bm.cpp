#include <benchmark/benchmark.h>

#include <algorithm>
#include <chrono>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "config.hpp"

#include "algorithms/limiters.hpp"

namespace {

constexpr size_t kLatencyCapacity = 10000;

constexpr double sparse_rate = 5.0;
constexpr int burst_size = 50;
constexpr double burst_probability = 0.02;

std::vector<avito_limiter::key_type> CrossShardKeys() {
  std::vector<avito_limiter::key_type> keys;
  keys.reserve(config::kShardCapacity * 2);
  for (std::size_t i = 0; i < config::kShardCapacity * 2; ++i) {
    keys.push_back("k" + std::to_string(i));
  }
  return keys;
}

const avito_limiter::key_type &
CrossShardKeyForThread(const std::vector<avito_limiter::key_type> &keys, int thread_index) {
  const std::size_t key_index = (thread_index % 2 == 0)
                                    ? config::kCrossShardKeyLastInFirstShard
                                    : config::kCrossShardKeyFirstInSecondShard;
  return keys[key_index];
}

} // namespace

template <class LimiterType>
static void BM_SparseBurst_Traffic(benchmark::State &state) {
  const double per_thread_sparse_rate = sparse_rate / static_cast<double>(state.threads());

  thread_local std::mt19937 gen(std::random_device{}());
  thread_local std::exponential_distribution<double> dist_sparse(1.0);
  thread_local std::bernoulli_distribution burst_trigger(burst_probability);

  dist_sparse.param(std::exponential_distribution<double>::param_type(per_thread_sparse_rate));

  std::vector<avito_limiter::key_type> keys = {"user_sparse_burst"};
  static LimiterType limiter{
      keys.begin(), keys.end(),
      std::tuple{config::kBurstCapacity, 1.0F}};

  std::vector<double> latencies;
  latencies.reserve(kLatencyCapacity);

  int burst_remaining = 0;
  for (auto _ : state) {
    if (burst_remaining > 0) {
      burst_remaining--;
    } else {
      const double delay_sec = dist_sparse(gen);
      std::this_thread::sleep_for(std::chrono::duration<double>(delay_sec));

      if (burst_trigger(gen)) {
        burst_remaining = burst_size;
      }
    }

    const auto req_start = std::chrono::high_resolution_clock::now();

    const bool allowed = limiter.Access(keys[0]);
    benchmark::DoNotOptimize(allowed);

    const auto req_end = std::chrono::high_resolution_clock::now();

    const double duration_ns = std::chrono::duration<double, std::nano>(req_end - req_start).count();
    latencies.push_back(duration_ns);
  }

  if (!latencies.empty()) {
    std::sort(latencies.begin(), latencies.end());
    const double p50 = latencies[static_cast<std::size_t>(latencies.size() * 0.50)];
    const double p99 = latencies[static_cast<std::size_t>(latencies.size() * 0.99)];

    state.counters["p50_ns"] = benchmark::Counter(p50, benchmark::Counter::kAvgThreads);
    state.counters["p99_ns"] = benchmark::Counter(p99, benchmark::Counter::kAvgThreads);
  }

  state.SetItemsProcessed(state.iterations());
}

template <class LimiterType>
static void BM_SparseBurst_CrossShard_Traffic(benchmark::State &state) {
  const double per_thread_sparse_rate = sparse_rate / static_cast<double>(state.threads());

  thread_local std::mt19937 gen(std::random_device{}());
  thread_local std::exponential_distribution<double> dist_sparse(1.0);
  thread_local std::bernoulli_distribution burst_trigger(burst_probability);

  dist_sparse.param(std::exponential_distribution<double>::param_type(per_thread_sparse_rate));

  static const std::vector<avito_limiter::key_type> keys = CrossShardKeys();
  static LimiterType limiter{
      keys.begin(), keys.end(),
      std::tuple{config::kBurstCapacity, 1.0F}};

  const avito_limiter::key_type &key = CrossShardKeyForThread(keys, static_cast<int>(state.thread_index()));

  std::vector<double> latencies;
  latencies.reserve(kLatencyCapacity);

  int burst_remaining = 0;
  for (auto _ : state) {
    if (burst_remaining > 0) {
      burst_remaining--;
    } else {
      const double delay_sec = dist_sparse(gen);
      std::this_thread::sleep_for(std::chrono::duration<double>(delay_sec));

      if (burst_trigger(gen)) {
        burst_remaining = burst_size;
      }
    }

    const auto req_start = std::chrono::high_resolution_clock::now();

    const bool allowed = limiter.Access(key);
    benchmark::DoNotOptimize(allowed);

    const auto req_end = std::chrono::high_resolution_clock::now();

    const double duration_ns = std::chrono::duration<double, std::nano>(req_end - req_start).count();
    latencies.push_back(duration_ns);
  }

  if (!latencies.empty()) {
    std::sort(latencies.begin(), latencies.end());
    const double p50 = latencies[static_cast<std::size_t>(latencies.size() * 0.50)];
    const double p99 = latencies[static_cast<std::size_t>(latencies.size() * 0.99)];

    state.counters["p50_ns"] = benchmark::Counter(p50, benchmark::Counter::kAvgThreads);
    state.counters["p99_ns"] = benchmark::Counter(p99, benchmark::Counter::kAvgThreads);
  }

  state.SetItemsProcessed(state.iterations());
}

using ShardedWinLimiterAligned =
    avito_limiter::ShardedWinLimiter<config::kShardCapacity, true>;
using ShardedWinLimiterCompact =
    avito_limiter::ShardedWinLimiter<config::kShardCapacity, false>;

BENCHMARK_TEMPLATE(BM_SparseBurst_Traffic, avito_limiter::MutexTokenLimiter)
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_SparseBurst_Traffic, avito_limiter::MutexWinLimiter)
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_SparseBurst_CrossShard_Traffic, ShardedWinLimiterAligned)
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_SparseBurst_CrossShard_Traffic, ShardedWinLimiterCompact)
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);
