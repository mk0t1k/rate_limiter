#pragma once

#include <cstddef>

#include <mutex>
#include <string>

namespace avito_limiter {

using key_type = std::string;

class IRateLimiter {
public:
  virtual bool Access(const key_type& key) = 0;

  virtual std::size_t GetNumAvail(
    const key_type& key) const noexcept = 0;

  virtual ~IRateLimiter() = default;
};

template<typename T>
concept RateLimAlgorithm = std::semiregular<T> && requires(T a, const T b) {
  {a.Access()} -> std::convertible_to<bool>;
  {b.GetNumAvail()} noexcept -> std::convertible_to<std::size_t>;
};
} // namespace avito_limiter