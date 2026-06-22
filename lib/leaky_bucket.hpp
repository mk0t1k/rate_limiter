#pragma once

#include <chrono>
#include <shared_mutex>
#include <optional>
#include <queue>
#include <thread>

#include "interface.hpp"

namespace avito_limiter {

class LeakyBucket final : public IRateShaper {
  struct QueueValue {
    std::promise<bool> prom;
    key_type key;
  };

  struct Config {
    std::size_t capacity = 0U;
    std::size_t cnt_remove_per_run = 0U;
    std::chrono::duration<double> wakeup_dur;
  };

  std::queue<QueueValue> queue_;
  std::shared_mutex mtx_;
  Config conf_;
  std::thread queue_update_;

  void UpdateQueue();

public:
  LeakyBucket(std::size_t capacity, std::size_t cnt_remove, double wake_up_sec);

  std::optional<future_type> AddRequest(const key_type& key) override;

  std::size_t GetNumAvail() const noexcept override;

  ~LeakyBucket();
};
} // namespace avito_limiter
