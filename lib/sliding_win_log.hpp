#pragma once

#include <cstddef>

#include <chrono>
#include <queue>

#include "alg_base.hpp"
#include "interface.hpp"
#include "ttl.hpp"

namespace avito_limiter {

template <typename Clock = std::chrono::steady_clock>
class SlidingWindowAlgo final : public AlgBase<SlidingWindowAlgo<Clock>, Clock> {
public:
  using clock_type = Clock;
private:
  using AlgBase_t = AlgBase<SlidingWindowAlgo<Clock>, Clock>;
  using TtlValue_t = TtlValue<Clock>;

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
  using time_val_t = typename AlgBase_t::value_type;

  SlidingWindowAlgo() = default;

  SlidingWindowAlgo(std::size_t capacity, float dur_sec, time_val_t ttl_val = TtlValue_t::kNoTtl) : 
    AlgBase_t{ttl_val}, capacity_{capacity}, time_offs_sec_{dur_sec} {}

  bool Verify() {
    Update();
    if(accesses_.size() == capacity_) {
      return false;
    }
    accesses_.push(clock_type::now());
    return true;
  }

  std::size_t GetAvail() const noexcept {
    Update();
    return static_cast<std::size_t>(capacity_ - accesses_.size());
  }
};
} // namespace avito_limiter
