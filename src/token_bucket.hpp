#pragma once 

#include <cstddef>

#include <algorithm>
#include <chrono>
#include <mutex>

#include "interface.hpp"

namespace avito_limiter {

class TokenBucket : public IrateLimiter {
public:
  using clock_type = std::chrono::steady_clock;
  using timepoint_type = decltype(clock_type::now());

  TokenBucket(std::size_t cap, std::size_t initial_cap, 
    double r_refill);

	void SetCapacity(std::size_t tgt);
	
	void SetRefillRate(double tk_per_sec);

  void Update() noexcept override;

  bool Receive(const key_type& key) override;
private:
  std::size_t capacity_ = 0U;
  double refill_rt_ = 0.0;

  timepoint_type last_;
};
} // namespace avito_limiter
