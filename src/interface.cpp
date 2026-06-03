#include "interface.hpp"

namespace avito_limiter {

std::size_t IrateLimiter::GetQueryLimit() const noexcept {
  return curr_limit_;
}
} // namespace avito_limiter
