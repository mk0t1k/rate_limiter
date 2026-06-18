#pragma once 

#include <cstddef>

#include <chrono>

#include "interface.hpp"

namespace avito_limiter {

class TokenBucketAlgo {
  using clock_type = std::chrono::steady_clock;
  using time_point = decltype(clock_type::now());

  mutable time_point last_access_;
  float refill_tok_sec_ = 0.0F;
  mutable float curr_tok_ = 0.0F;
  float capacity_ = 0.0F;

  void Update() const noexcept;
public:
  TokenBucketAlgo();
  
  TokenBucketAlgo(float v_refill, std::size_t capacity);

  bool Access() noexcept;

  std::size_t GetNumAvail() const noexcept;
};
} // namespace avito_limiter
