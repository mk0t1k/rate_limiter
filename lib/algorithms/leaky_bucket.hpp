#pragma once

#include <atomic>
#include <chrono>
#include <shared_mutex>
#include <optional>
#include <queue>
#include <thread>

#include "interface.hpp"

namespace avito_limiter {

class LeakyBucketShaper final : public IRateShaper {
  struct QueueValue {
    key_type key;
    std::promise<bool> prom;
  };

  struct Config {
    std::size_t capacity = 0U;
    std::size_t cnt_remove_per_run = 0U;
    std::chrono::duration<double> wakeup_dur;
  };

  std::queue<QueueValue> queue_;
  mutable std::shared_mutex mtx_;
  Config conf_;
  std::atomic_bool run_queue_thread_;
  std::thread queue_update_;

  void UpdateQueue();

  void RunQueueThread();

public:
  LeakyBucketShaper(std::size_t capacity, std::size_t cnt_remove, double wake_up_sec);

  std::optional<future_type> AddRequest(const key_type& key) override;

  std::size_t GetNumAvail() const noexcept override;

  ~LeakyBucketShaper();
};
} // namespace avito_limiter
