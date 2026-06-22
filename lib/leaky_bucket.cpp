#include "leaky_bucket.hpp"

#include <chrono>
#include <thread>

namespace avito_limiter {

void LeakyBucket::UpdateQueue() {
  std::unique_lock ul(mtx_);
  for(std::size_t _ = 0U; _ < conf_.cnt_remove_per_run && !queue_.empty(); ++_) {
    QueueValue val = std::move(queue_.front());
    queue_.pop();
    val.prom.set_value(true);
  }
}

LeakyBucket::LeakyBucket(
  std::size_t capacity, std::size_t cnt_remove, double wake_up_sec) :
  conf_{.capacity=capacity, .cnt_remove_per_run=cnt_remove, 
    .wakeup_dur=std::chrono::duration<double>(wake_up_sec)},
  queue_update_{&LeakyBucket::UpdateQueue, this} {}

std::optional<LeakyBucket::future_type> LeakyBucket::AddRequest(
  const key_type& key) {
  std::shared_lock sl(mtx_);
  if(queue_.size() + 1Z > conf_.capacity) {
    return std::nullopt;
  }
  sl.unlock();
  std::unique_lock ul(mtx_);
  std::promise<bool> pr;
  future_type fut = pr.get_future();
  return fut;
}

LeakyBucket::~LeakyBucket() {
  queue_update_.join();
}
} // namespace avito_limiter
