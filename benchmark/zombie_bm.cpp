#include <benchmark/benchmark.h>

#include <atomic>
#include <mutex>
#include <random>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <string>
#include <optional>

#include "config.hpp"

#include "algorithms/limiters.hpp"

namespace {

  constexpr size_t kLatencyCapacity = 10000;

} // namespace

template <class LimiterType>
static void BM_Zombie_Traffic(benchmark::State& state) {

  double target_rate = static_cast<double>(state.range(0));
  double per_thread_rate =
      target_rate / static_cast<double>(state.threads());

  thread_local std::mt19937 gen(std::random_device{}());
  thread_local std::exponential_distribution<double> dist(1.0);
  dist.param(
      std::exponential_distribution<double>::param_type(per_thread_rate));

  static std::atomic<size_t> next_key_id{0};

  static std::vector<avito_limiter::key_type> empty_keys;
  static LimiterType limiter{
      empty_keys.begin(), empty_keys.end(),
      std::tuple{config::kBurstCapacity, 1.0F},
      std::optional<double>{5.0},
      std::optional<double>{1.0},
      true};

  thread_local size_t my_key_id = next_key_id.fetch_add(
      1, std::memory_order_relaxed);
  thread_local avito_limiter::key_type cached_key =
      "z_" + std::to_string(my_key_id);
  thread_local int req_count = config::kZombieReqsPerKey;

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

    if (req_count >= config::kZombieReqsPerKey) {
      my_key_id = next_key_id.fetch_add(1, std::memory_order_relaxed);
      cached_key = "z_" + std::to_string(my_key_id);
      limiter.Emplace(cached_key);
      req_count = 0;
    }

    auto start = std::chrono::high_resolution_clock::now();

    bool allowed = limiter.Access(cached_key);
    benchmark::DoNotOptimize(allowed);

    auto end = std::chrono::high_resolution_clock::now();

    double duration_ns =
        std::chrono::duration<double, std::nano>(end - start).count();
    latencies.push_back(duration_ns);

    ++req_count;
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

using ShardedWinLimiterAligned =
    avito_limiter::ShardedWinLimiter<config::kShardCapacity, true>;
using ShardedWinLimiterCompact =
    avito_limiter::ShardedWinLimiter<config::kShardCapacity, false>;

BENCHMARK_TEMPLATE(BM_Zombie_Traffic, avito_limiter::MutexTokenLimiter)
    ->Arg(static_cast<int64_t>(config::kZombieTargetRatePerSec))
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_Zombie_Traffic, avito_limiter::MutexWinLimiter)
    ->Arg(static_cast<int64_t>(config::kZombieTargetRatePerSec))
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_Zombie_Traffic, ShardedWinLimiterAligned)
    ->Arg(static_cast<int64_t>(config::kZombieTargetRatePerSec))
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);

BENCHMARK_TEMPLATE(BM_Zombie_Traffic, ShardedWinLimiterCompact)
    ->Arg(static_cast<int64_t>(config::kZombieTargetRatePerSec))
    ->Threads(config::kNumThreads)
    ->Repetitions(config::kRepetitions)
    ->MinTime(config::kMinTestDurationSec);
