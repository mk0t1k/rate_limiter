#pragma once

#include <cstddef>

#include <chrono>
#include <queue>

#include "interface.hpp"

namespace avito_limiter {

class SlidingWindowAlgo {
  using clock_type = std::chrono::steady_clock;
  using time_point = decltype(clock_type::now());
  using duration_type = typename clock_type::duration;

  using stored_type = std::priority_queue<time_point, 
    std::vector<time_point>, std::greater<time_point>>;
  mutable stored_type accesses_;
  std::size_t capacity_ = 0U;
  float time_offs_sec_ = 0.0F;

  void Update() const;
public:
  SlidingWindowAlgo() = default;

  SlidingWindowAlgo(
    std::size_t capacity, float dur_sec);

  bool Access();

  std::size_t GetNumAvail() const noexcept;
};
} // namespace avito_limiter
