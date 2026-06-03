#pragma once

#include <cstddef>

#include <mutex>
#include <string>

namespace avito_limiter {

using key_type = std::string;

class IRateLimiter {
public:
  std::size_t GetQueryLimit() const noexcept;

  virtual void Update() noexcept = 0;

  virtual bool Receive(const key_type& key) = 0;
  
  virtual ~IRateLimiter() = default;
protected:
  std::size_t curr_limit_ = 0U;
  std::mutex intfc_mutex_;
};
} // namespace avito_limiter