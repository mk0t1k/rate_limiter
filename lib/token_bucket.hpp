#pragma once 

#include <cstddef>

#include <chrono>

#include "alg_base.hpp"
#include "interface.hpp"
#include "ttl.hpp"

namespace avito_limiter {

namespace {
  constexpr float kTokBuckEpsilon = 1e-4;
  constexpr float kTokBuckUnitPos = 1.0F - kTokBuckEpsilon;
} // namespace

template <typename Clock = std::chrono::steady_clock>
class TokenBucketAlgo final: public AlgBase<TokenBucketAlgo<Clock>, Clock> {
public:
  using clock_type = Clock;
private:
  using TtlValue_t = TtlValue<Clock>; 
  using AlgBase_t = AlgBase<TokenBucketAlgo<Clock>, Clock>;

  using time_point = decltype(clock_type::now());

  mutable time_point last_access_;
  float refill_tok_sec_ = 0.0F;
  mutable float curr_tok_ = 0.0F;
  float capacity_ = 0.0F;

  void Update() const noexcept {
    time_point curr = clock_type::now();
    float dur_sec = std::chrono::duration<float>(curr - last_access_).count();
    last_access_ = curr;
    float cnt_refill = std::min(capacity_, refill_tok_sec_ * dur_sec);
    curr_tok_ = std::min(capacity_, cnt_refill + curr_tok_);
  }

public:
  using time_val_t = typename AlgBase_t::value_type;

  TokenBucketAlgo() : last_access_{clock_type::now()} {}
  
  TokenBucketAlgo(float v_refill, std::size_t capacity, time_val_t ttl_val=TtlValue_t::kNoTtl) : 
    AlgBase_t{ttl_val},
    last_access_{clock_type::now()}, 
    refill_tok_sec_{v_refill} 
    {
      capacity_ = static_cast<float>(capacity);
      curr_tok_ = capacity_;
    }

  bool Verify() noexcept {
    Update();
    if(curr_tok_ >= kTokBuckUnitPos) {
      curr_tok_ -= 1.0F;
      return true;
    }
    return false;
  }

  std::size_t GetAvail() const noexcept {
    Update();
    return static_cast<std::size_t>(curr_tok_);
  }
};
} // namespace avito_limiter
