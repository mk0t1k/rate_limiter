#include <benchmark/benchmark.h>

#include <atomic>
#include <mutex>
#include <random>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
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

  auto CrossShardKeyForThread(const std::vector<avito_limiter::key_type>& keys,int thread_index)
  -> const avito_limiter::key_type& 
  {
    std::size_t key_index;
      if (thread_index % 2 == 0)
        key_index = config::kCrossShardKeyLastInFirstShard;
      else
        key_index = config::kCrossShardKeyFirstInSecondShard;
    return keys[key_index];
  }

} // namespace

template <class LimiterType>
static void BM_Uniform_Traffic(benchmark::State& state) {

  double target_rate = static_cast<double>(state.range(0));
  double per_thread_rate = target_rate / static_cast<double>(state.threads());

  thread_local std::mt19937 gen(std::random_device{}());
  thread_local std::exponential_distribution<double> dist(1.0);
  dist.param(std::exponential_distribution<double>::param_type(per_thread_rate));

  std::vector<avito_limiter::key_type> keys = {"user_uniform"};
  static LimiterType limiter{
      keys.begin(), keys.end(),
      std::tuple{config::kBurstCapacity, 1.0F}};

  static std::mutex s_mtx;
  static std::vector<std::vector<double>> s_per_thread_lats;
  static std::atomic<int> s_done{0};
  {
    std::lock_guard lk(s_mtx);
    if (s_per_thread_lats.size() != static_cast<size_t>(state.threads())) {
      s_per_thread_lats.resize(state.threads());
    }
    s_per_thread_lats[state.thread_index()].clear();
  }
  s_done.store(0, std::memory_order_release);

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

  {
    std::lock_guard lk(s_mtx);
    s_per_thread_lats[state.thread_index()] = std::move(latencies);
  }

  int done = s_done.fetch_add(1, std::memory_order_acq_rel) + 1;
  if (done == state.threads()) {
    std::vector<double> all;
    for (int i = 0; i < state.threads(); ++i) {
      all.insert(all.end(), s_per_thread_lats[i].begin(),
                 s_per_thread_lats[i].end());
    }
    if (!all.empty()) {
      std::sort(all.begin(), all.end());
      size_t sz = all.size();
      double p50 = all[static_cast<size_t>(sz * 0.50)];
      double p99 = all[static_cast<size_t>(sz * 0.99)];

      state.counters["p50_ns"] = benchmark::Counter(p50);
      state.counters["p99_ns"] = benchmark::Counter(p99);
    }
  }

  state.SetItemsProcessed(state.iterations());
}

template <class LimiterType>
static void BM_Uniform_CrossShard_Traffic(benchmark::State& state) {

  double target_rate = static_cast<double>(state.range(0));
  double per_thread_rate = target_rate / static_cast<double>(state.threads());

  thread_local std::mt19937 gen(std::random_device{}());
  thread_local std::exponential_distribution<double> dist(1.0);
  dist.param(std::exponential_distribution<double>::param_type(per_thread_rate));

  static const std::vector<avito_limiter::key_type> keys = CrossShardKeys();
  static LimiterType limiter{
      keys.begin(), keys.end(),
      std::tuple{config::kBurstCapacity, 1.0F}};

  const avito_limiter::key_type& key =
    CrossShardKeyForThread(keys, static_cast<int>(state.thread_index()));

  static std::mutex s_mtx_cs;
  static std::vector<std::vector<double>> s_per_thread_lats_cs;
  static std::atomic<int> s_done_cs{0};
  {
    std::lock_guard lk(s_mtx_cs);
    if (s_per_thread_lats_cs.size() != static_cast<size_t>(state.threads())) {
      s_per_thread_lats_cs.resize(state.threads());
    }
    s_per_thread_lats_cs[state.thread_index()].clear();
  }
  s_done_cs.store(0, std::memory_order_release);

  std::vector<double> latencies;
  latencies.reserve(kLatencyCapacity);

  for (auto _ : state) {

    double delay_sec = dist(gen);
    std::this_thread::sleep_for(std::chrono::duration<double>(delay_sec));

    auto start = std::chrono::high_resolution_clock::now();

    bool allowed = limiter.Access(key);
    benchmark::DoNotOptimize(allowed);

    auto end = std::chrono::high_resolution_clock::now();

    double duration_ns = std::chrono::duration<double, std::nano>(end - start).count();
    latencies.push_back(duration_ns);
  }

  {
    std::lock_guard lk(s_mtx_cs);
    s_per_thread_lats_cs[state.thread_index()] = std::move(latencies);
  }

  int done = s_done_cs.fetch_add(1, std::memory_order_acq_rel) + 1;
  if (done == state.threads()) {
    std::vector<double> all;
    for (int i = 0; i < state.threads(); ++i) {
      all.insert(all.end(), s_per_thread_lats_cs[i].begin(),
                 s_per_thread_lats_cs[i].end());
    }
    if (!all.empty()) {
      std::sort(all.begin(), all.end());
      size_t sz = all.size();
      double p50 = all[static_cast<size_t>(sz * 0.50)];
      double p99 = all[static_cast<size_t>(sz * 0.99)];

      state.counters["p50_ns"] = benchmark::Counter(p50);
      state.counters["p99_ns"] = benchmark::Counter(p99);
    }
  }

  state.SetItemsProcessed(state.iterations());
}

using ShardedWinLimiterAligned =
  avito_limiter::ShardedWinLimiter<config::kShardCapacity, true>;
using ShardedWinLimiterCompact =
  avito_limiter::ShardedWinLimiter<config::kShardCapacity, false>;

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

BENCHMARK_TEMPLATE(BM_Uniform_CrossShard_Traffic, ShardedWinLimiterAligned)
    ->RangeMultiplier(2)
    ->Range(10, config::kTargetRatePerSec)
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_Uniform_CrossShard_Traffic, ShardedWinLimiterCompact)
    ->RangeMultiplier(2)
    ->Range(10, config::kTargetRatePerSec)
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);