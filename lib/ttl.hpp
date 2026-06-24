#pragma once

#include <cmath>

#include <chrono>
#include <compare>
#include <optional>
#include <stdexcept>

namespace avito_limiter {

template <typename Clock = typename std::chrono::steady_clock>
class TtlValue final {
public:
  using clock_type = Clock;
  using time_point = decltype(clock_type::now());
  using offset_type = double;
private:

  std::optional<time_point> expire_time_;

public:
  using value_type = std::optional<offset_type>;
  static constexpr value_type kNoTtl = std::nullopt;

  TtlValue() = default;

  explicit TtlValue(value_type time_sec) {
    Set(time_sec);
  }

  void Set(value_type time_sec) noexcept {
    if(time_sec) {
      offset_type val = time_sec.value();
      if(val != val || std::isinf(val)) {
        expire_time_ = std::nullopt;
        return;
      }
      auto res = clock_type::now() + std::chrono::duration<offset_type>{
        time_sec.value()};
      expire_time_ = std::chrono::time_point_cast<typename clock_type::duration>(
              res);  
    } else {
      expire_time_ = std::nullopt;
    }
  }

  time_point ExpireTime() const noexcept {
    if(expire_time_) {
      return expire_time_.value();
    }
    return clock_type::now();
  }

  value_type GetTtl() noexcept {
    if(!static_cast<bool>(expire_time_)) {
      return std::nullopt;
    }
    auto dur = ExpireTime() - clock_type::now();
    auto dur_sec = 
      std::chrono::duration_cast<std::chrono::duration<offset_type>>(dur);
    return dur_sec.count();
  }

  operator bool() const noexcept {
    return !static_cast<bool>(expire_time_) || clock_type::now() <= ExpireTime();
  }

  bool operator==(const TtlValue& other) const noexcept {
    bool is_infinite = !static_cast<bool>(expire_time_);
    bool is_other_infinite = !static_cast<bool>(other.expire_time_);
    if(is_infinite && is_other_infinite) {
      return true;
    }
    if(is_infinite || is_other_infinite) {
      return false;
    }
    return ExpireTime() == other.ExpireTime();
  }

  std::weak_ordering operator<=>(const TtlValue& other) const noexcept {
    bool is_infinite = !static_cast<bool>(expire_time_);
    bool is_other_infinite = !static_cast<bool>(other.expire_time_);
    if(is_infinite && is_other_infinite) {
      return std::weak_ordering::equivalent;
    }
    if(is_infinite) {
      return std::weak_ordering::greater;
    }
    if(is_other_infinite) {
      return std::weak_ordering::less;
    }
    return ExpireTime() <=> other.ExpireTime();
  }
};
}  // namespace avito_limiter
