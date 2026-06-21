#pragma once 

#include <cstddef>

#include <chrono>

#include "alg_base.hpp"
#include "interface.hpp"
#include "ttl.hpp"

namespace avito_limiter {

class TokenBucketAlgo final: public AlgBase<TokenBucketAlgo> {
  using clock_type = std::chrono::steady_clock;
  using time_point = decltype(clock_type::now());

  mutable time_point last_access_;
  float refill_tok_sec_ = 0.0F;
  mutable float curr_tok_ = 0.0F;
  float capacity_ = 0.0F;

  void Update() const noexcept;
public:
  using time_val_t = typename AlgBase::value_type;

  TokenBucketAlgo();
  
  TokenBucketAlgo(float v_refill, std::size_t capacity);

  bool Verify() noexcept;

  std::size_t GetAvail() const noexcept;
};
} // namespace avito_limiter
