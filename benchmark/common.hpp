#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "algorithms/limiters.hpp"
#include "config.hpp"

namespace bench {

inline constexpr std::size_t kLatencyCapacity = 10000;

inline std::vector<avito_limiter::key_type> CrossShardKeys() {
  std::vector<avito_limiter::key_type> keys;
  keys.reserve(config::kShardCapacity * 2);
  for (std::size_t i = 0; i < config::kShardCapacity * 2; ++i) {
    keys.push_back("k" + std::to_string(i));
  }
  return keys;
}

inline const avito_limiter::key_type& CrossShardKeyForThread(
    const std::vector<avito_limiter::key_type>& keys, int thread_index) {
  const std::size_t key_index = (thread_index % 2 == 0)
      ? config::kCrossShardKeyLastInFirstShard
      : config::kCrossShardKeyFirstInSecondShard;
  return keys[key_index];
}

inline void PublishLatencyPercentiles(
    benchmark::State& state,
    const std::vector<std::vector<double>>& per_thread_lats,
    int num_threads) {
  std::vector<double> all;
  for (int i = 0; i < num_threads; ++i) {
    all.insert(all.end(), per_thread_lats[i].begin(), per_thread_lats[i].end());
  }
  if (all.empty()) {
    return;
  }
  std::sort(all.begin(), all.end());
  const std::size_t sz = all.size();
  const double p50 = all[static_cast<std::size_t>(sz * 0.50)];
  const double p99 = all[static_cast<std::size_t>(sz * 0.99)];
  state.counters["p50_ns"] = benchmark::Counter(p50);
  state.counters["p99_ns"] = benchmark::Counter(p99);
}

inline void BeginLatencyCollection(
    benchmark::State& state,
    std::mutex& mtx,
    std::vector<std::vector<double>>& per_thread_lats,
    std::atomic<int>& done) {
  {
    std::lock_guard lk(mtx);
    if (per_thread_lats.size() != static_cast<std::size_t>(state.threads())) {
      per_thread_lats.resize(state.threads());
    }
    per_thread_lats[state.thread_index()].clear();
  }
  done.store(0, std::memory_order_release);
}

inline void FinishLatencyCollection(
    benchmark::State& state,
    std::mutex& mtx,
    std::vector<std::vector<double>>& per_thread_lats,
    std::atomic<int>& done,
    std::vector<double> latencies) {
  {
    std::lock_guard lk(mtx);
    per_thread_lats[state.thread_index()] = std::move(latencies);
  }
  const int finished = done.fetch_add(1, std::memory_order_acq_rel) + 1;
  if (finished == state.threads()) {
    PublishLatencyPercentiles(state, per_thread_lats, state.threads());
  }
}

} // namespace bench
