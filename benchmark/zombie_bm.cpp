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

  std::vector<avito_limiter::key_type> GenZombieKeys() {
    std::vector<avito_limiter::key_type> keys;
    keys.reserve(config::kZombieKeysCount);
    for (int i = 0; i < config::kZombieKeysCount; ++i) {
      keys.push_back("z_" + std::to_string(i));
    }
    return keys;
  }

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

  static const std::vector<avito_limiter::key_type> keys = GenZombieKeys();
  static LimiterType limiter{keys.begin(), keys.end()};

  thread_local size_t key_index = 0;
  thread_local int req_count = 0;

  std::vector<double> latencies;
  latencies.reserve(kLatencyCapacity);

  for (auto _ : state) {

    double delay_sec = dist(gen);
    std::this_thread::sleep_for(std::chrono::duration<double>(delay_sec));

    auto start = std::chrono::high_resolution_clock::now();

    bool allowed = limiter.Access(keys[key_index]);
    benchmark::DoNotOptimize(allowed);

    auto end = std::chrono::high_resolution_clock::now();

    double duration_ns =
        std::chrono::duration<double, std::nano>(end - start).count();
    latencies.push_back(duration_ns);

    ++req_count;
    if (req_count >= config::kZombieReqsPerKey) {
      key_index = (key_index + 1) % config::kZombieKeysCount;
      req_count = 0;
    }
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
