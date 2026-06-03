#pragma once

#include <cstddef>

#include <mutex>
#include <string>

#include "interface.hpp"

namespace avito_limiter {

class SlidingWindowLog : public IRateLimiter {
public:
	void SetCapacity(std::size_t tgt);
	
	void SetTimeWidth(double cnt_sec);

  void Update() noexcept override;

  bool Receive(const key_type& key) override;
};
} // namespace avito_limiter
