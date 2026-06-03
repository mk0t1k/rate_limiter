#include "token_bucket.hpp"

#include <cstddef>

#include <algorithm>
#include <chrono>
#include <mutex>

namespace avito_limiter {

TokenBucket::TokenBucket(std::size_t cap, std::size_t initial_cap,
                         double r_refill)
    : capacity_{cap}, refill_rt_{r_refill}, last_{clock_type::now()} {
  curr_limit_ = std::min(capacity_, initial_cap);
}

void TokenBucket::SetCapacity(std::size_t tgt) {
  std::lock_guard lock(intfc_mutex_);
  capacity_ = tgt;
  if (curr_limit_ > tgt) {
    curr_limit_ = tgt;
  }
}

void TokenBucket::SetRefillRate(double tk_per_sec) {
  std::lock_guard lock(intfc_mutex_);
  refill_rt_ = tk_per_sec;
}

void TokenBucket::Update() noexcept {
  timepoint_type curr = clock_type::now();
  double elapsed_sec = std::chrono::duration<double>(curr - last_).count();
  curr_limit_ =
      std::min(curr_limit_ + std::size_t(elapsed_sec * refill_rt_), capacity_);
  last_ = curr;
}

bool TokenBucket::Receive(const key_type& key) {
  std::lock_guard lock(intfc_mutex_);
  if (curr_limit_) {
    --curr_limit_;
    return true;
  }
  return false;
}
}  // namespace avito_limiter
