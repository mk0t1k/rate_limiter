#include "leaky_bucket.hpp"

#include <chrono>
#include <thread>

namespace avito_limiter {

void LeakyBucketShaper::UpdateQueue() {
  std::unique_lock ul(mtx_);
  for(std::size_t _ = 0U; _ < conf_.cnt_remove_per_run && !queue_.empty(); ++_) {
    QueueValue val = std::move(queue_.front());
    queue_.pop();
    val.prom.set_value(true);
  }
}

void LeakyBucketShaper::RunQueueThread() {
  while (run_queue_thread_.load(std::memory_order_relaxed)) {
    std::this_thread::sleep_for(conf_.wakeup_dur);
    UpdateQueue();
  }
}

LeakyBucketShaper::LeakyBucketShaper(
  std::size_t capacity, std::size_t cnt_remove, double wake_up_sec) :
  conf_{.capacity=capacity, .cnt_remove_per_run=cnt_remove, 
    .wakeup_dur=std::chrono::duration<double>(wake_up_sec)},
  run_queue_thread_{true},
  queue_update_{&LeakyBucketShaper::RunQueueThread, this} {}

std::optional<LeakyBucketShaper::future_type> LeakyBucketShaper::AddRequest(
  const key_type& key) {
  std::shared_lock sl(mtx_);
  if(queue_.size() + 1Z > conf_.capacity) {
    return std::nullopt;
  }
  sl.unlock();
  std::unique_lock ul(mtx_);
  queue_.emplace(key);
  future_type fut = queue_.back().prom.get_future();
  return fut;
}

std::size_t LeakyBucketShaper::GetNumAvail() const noexcept {
  std::shared_lock sl(mtx_);
  return conf_.capacity - queue_.size();
}

LeakyBucketShaper::~LeakyBucketShaper() {
  run_queue_thread_.store(false, std::memory_order_relaxed);
  queue_update_.join();
}
} // namespace avito_limiter
