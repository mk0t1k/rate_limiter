#pragma once

#include <cstddef>

#include <chrono>
#include <queue>

#include "alg_base.hpp"
#include "interface.hpp"
#include "ttl.hpp"

namespace avito_limiter {

class SlidingWindowAlgo final : public AlgBase<SlidingWindowAlgo> {
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
  using time_val_t = typename AlgBase::value_type;

  SlidingWindowAlgo() = default;

  SlidingWindowAlgo(
    std::size_t capacity, float dur_sec, time_val_t ttl_val=TtlValue::kNoTtl);

  bool Verify();

  std::size_t GetAvail() const noexcept;
};
} // namespace avito_limiter
