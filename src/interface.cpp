#include "interface.hpp"

namespace avito_limiter {

std::size_t IRateLimiter::GetQueryLimit() const noexcept {
  return curr_limit_;
}
} // namespace avito_limiter
