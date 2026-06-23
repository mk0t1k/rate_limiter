#include <benchmark/benchmark.h>

#include <atomic>
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
  thread_local int req_count = 0;

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
