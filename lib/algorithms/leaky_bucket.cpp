#include "leaky_bucket.hpp"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stop_token>
#include <thread>

namespace avito_limiter {

void LeakyBucketShaper::UpdateQueue() {
  for(std::size_t _ = 0U; _ < conf_.cnt_remove_per_run && !queue_.empty(); ++_) {
    QueueValue val = std::move(queue_.front());
    queue_.pop();
    val.prom.set_value(true);
  }
}

void LeakyBucketShaper::RunQueueThread(std::stop_token stoken) {
  while (!stoken.stop_requested()) {
    std::unique_lock ul(mtx_);
    cva_.wait_for(ul, conf_.wakeup_dur, [this]{ return force_trigger_ || stop_flag_; });

    if(force_trigger_) {
      force_trigger_ = false;
    }
    if(stop_flag_) {
      break;
    }

    UpdateQueue();
  }
}

void LeakyBucketShaper::ForceTrigger() {
  std::unique_lock lock(mtx_);
  force_trigger_ = true;
  cva_.notify_one();
}

LeakyBucketShaper::LeakyBucketShaper(
  std::size_t capacity, std::size_t cnt_remove, double wake_up_sec) :
  conf_{.capacity=capacity, .cnt_remove_per_run=cnt_remove, 
    .wakeup_dur=std::chrono::duration<double>(wake_up_sec)},
  queue_update_{&LeakyBucketShaper::RunQueueThread, this} {}

std::optional<LeakyBucketShaper::future_type> LeakyBucketShaper::AddRequest(
  const key_type& key) {
  std::unique_lock sl(mtx_);
  if(queue_.size() + 1Z > conf_.capacity) {
    return std::nullopt;
  }
  queue_.emplace(key);
  future_type fut = queue_.back().prom.get_future();
  return fut;
}

std::size_t LeakyBucketShaper::GetNumAvail() const noexcept {
  std::shared_lock sl(mtx_);
  return conf_.capacity - queue_.size();
}

LeakyBucketShaper::~LeakyBucketShaper() {
  {
    std::unique_lock lock(mtx_);
    stop_flag_ = true;
    cva_.notify_one();
  }
  if(queue_update_.joinable()) {
    queue_update_.join();
  }
}
} // namespace avito_limiter