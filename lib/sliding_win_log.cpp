#include "sliding_win_log.hpp"

#include <cstddef>

#include <chrono>
#include <queue>

#include "interface.hpp"

namespace avito_limiter {

void SlidingWindowAlgo::Update() const {
  time_point curr = clock_type::now();
  std::chrono::duration<float> offset_duration(time_offs_sec_);
  duration_type casted_offset = 
    std::chrono::duration_cast<duration_type>(offset_duration);
  time_point cull_point = curr - casted_offset;
  while (accesses_.size() && accesses_.top() < cull_point) {
    accesses_.pop();
  }
}

SlidingWindowAlgo::SlidingWindowAlgo(
  std::size_t capacity, float dur_sec, time_val_t ttl_val) : 
  AlgBase{ttl_val}, capacity_{capacity}, time_offs_sec_{dur_sec} {}

bool SlidingWindowAlgo::Verify() {
  Update();
  if(accesses_.size() == capacity_) {
    return false;
  }
  accesses_.push(clock_type::now());
  return true;
}

std::size_t SlidingWindowAlgo::GetAvail() const noexcept {
  Update();
  return static_cast<std::size_t>(capacity_ - accesses_.size());
}
} // namespace avito_limiter
