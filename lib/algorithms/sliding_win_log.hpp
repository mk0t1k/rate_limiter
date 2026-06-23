#pragma once

#include <cstddef>

#include <chrono>
#include <queue>

namespace avito_limiter {

template <typename Derived, typename Clock>
class SlidingWindowAlgo {
public:
  using clock_type = Clock;
private:
  using time_point = decltype(clock_type::now());
  using duration_type = typename clock_type::duration;

  using stored_type = std::priority_queue<time_point, 
    std::vector<time_point>, std::greater<time_point>>;
  mutable stored_type accesses_;
  std::size_t capacity_ = 0U;
  float time_offs_sec_ = 0.0F;

  void Update() const {
    time_point curr = clock_type::now();
    std::chrono::duration<float> offset_duration(time_offs_sec_);
    duration_type casted_offset = 
      std::chrono::duration_cast<duration_type>(offset_duration);
    time_point cull_point = curr - casted_offset;
    while (accesses_.size() && accesses_.top() < cull_point) {
      accesses_.pop();
    }
  }
public:

  SlidingWindowAlgo() = default;

  SlidingWindowAlgo(std::size_t capacity, float dur_sec) : 
    capacity_{capacity}, time_offs_sec_{dur_sec} {}

  bool Access() {
    if(static_cast<Derived*>(this)->IsExpired()) {
      return false;
    }
    static_cast<Derived*>(this)->Prolong();
    Update();
    if(accesses_.size() == capacity_) {
      return false;
    }
    accesses_.push(clock_type::now());
    return true;
  }

  std::size_t GetNumAvail() const noexcept {
    if(static_cast<const Derived*>(this)->IsExpired()) {
      return 0Z;
    }
    Update();
    return static_cast<std::size_t>(capacity_ - accesses_.size());
  }
};
} // namespace avito_limiter
