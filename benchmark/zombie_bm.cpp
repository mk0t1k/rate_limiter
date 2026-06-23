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
      config::kKeyTTL,
      1.0,
      true};

  thread_local size_t my_key_id = next_key_id.fetch_add(
      1, std::memory_order_relaxed);
  thread_local avito_limiter::key_type cached_key =
      "z_" + std::to_string(my_key_id);
  thread_local int req_count = 0;

  static std::mutex s_mtx;
  static std::vector<std::vector<double>> s_per_thread_lats;
  static std::atomic<int> s_done{0};
  bench::BeginLatencyCollection(state, s_mtx, s_per_thread_lats, s_done);

  std::vector<double> latencies;
  latencies.reserve(bench::kLatencyCapacity);

  limiter.AddKey(cached_key, config::kKeyTTL);

  for (auto _ : state) {

    double delay_sec = dist(gen);
    std::this_thread::sleep_for(std::chrono::duration<double>(delay_sec));

    if (req_count >= config::kZombieReqsPerKey) {
      my_key_id = next_key_id.fetch_add(1, std::memory_order_relaxed);
      cached_key = "z_" + std::to_string(my_key_id);
      limiter.AddKey(cached_key, config::kKeyTTL);
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

  bench::FinishLatencyCollection(state, s_mtx, s_per_thread_lats, s_done,
                                 std::move(latencies));

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
