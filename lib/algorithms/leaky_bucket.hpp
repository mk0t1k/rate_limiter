#pragma once

#include <chrono>
#include <condition_variable>
#include <shared_mutex>
#include <optional>
#include <queue>
#include <stop_token>
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
  std::condition_variable_any cva_;
  Config conf_;
  bool force_trigger_;
  std::jthread queue_update_;

  void UpdateQueue();

  void RunQueueThread(std::stop_token stoken);

public:
  LeakyBucketShaper(std::size_t capacity, std::size_t cnt_remove, double wake_up_sec);

  void ForceTrigger();

  std::optional<future_type> AddRequest(const key_type& key) override;

  std::size_t GetNumAvail() const noexcept override;

  ~LeakyBucketShaper() = default;
};
} // namespace avito_limiter
