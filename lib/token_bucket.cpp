#include "token_bucket.hpp"

#include <cstddef>

#include <algorithm>
#include <chrono>

#include "interface.hpp"

namespace {

constexpr float kTokBuckEpsilon = 1e-4;
constexpr float kTokBuckUnitPos = 1.0F - kTokBuckEpsilon;
} // namespace

namespace avito_limiter {

void TokenBucketAlgo::Update() const noexcept {
  time_point curr = clock_type::now();
  float dur_sec = std::chrono::duration<float>(curr - last_access_).count();
  last_access_ = curr;
  float cnt_refill = std::min(capacity_, refill_tok_sec_ * dur_sec);
  curr_tok_ = std::min(capacity_, cnt_refill + curr_tok_);
}

TokenBucketAlgo::TokenBucketAlgo() : last_access_{clock_type::now()} {}

TokenBucketAlgo::TokenBucketAlgo(float v_refill, std::size_t capacity) : 
  last_access_{clock_type::now()}, refill_tok_sec_{v_refill} {
  capacity_ = static_cast<float>(capacity);
  curr_tok_ = capacity_;
}

bool TokenBucketAlgo::Verify() noexcept {
  Update();
  if(curr_tok_ >= kTokBuckUnitPos) {
    curr_tok_ -= 1.0F;
    return true;
  }
  return false;
}

std::size_t TokenBucketAlgo::GetAvail() const noexcept {
  Update();
  return static_cast<std::size_t>(curr_tok_);
}
}  // namespace avito_limiter
