#pragma once

#include <chrono>
#include <compare>

namespace avito_limiter {

template <typename Clock = typename std::chrono::steady_clock>
class TtlValue final {
public:
  using clock_type = Clock;
  using time_point = decltype(clock_type::now());
  using offset_type = double;
private:

  bool is_infinite_ = true;
  offset_type ttl_sec_;
  time_point time_created_;

public:
  using value_type = std::optional<offset_type>;
  static constexpr value_type kNoTtl = std::nullopt;

  TtlValue() = default;

  explicit TtlValue(value_type time_sec) {
    Set(time_sec);
  }

  void Set(value_type time_sec) noexcept {
    time_created_ = clock_type::now();
    if(time_sec) {
      ttl_sec_ = time_sec.value();
      is_infinite_ = false;    
    } else {
      is_infinite_ = true;
    }
  }

  time_point ExpireTime() const noexcept {
    auto res = time_created_ + std::chrono::duration<offset_type>{ttl_sec_};
    time_point target_time = 
          std::chrono::time_point_cast<typename clock_type::duration>(
              res
          );
    return target_time;
  }

  value_type GetTtl() noexcept {
    if(is_infinite_) {
      return std::nullopt;
    }
    auto dur = ExpireTime() - clock_type::now();
    auto dur_sec = 
      std::chrono::duration_cast<std::chrono::duration<offset_type>>(dur);
    return dur_sec.count();
  }

  operator bool() const noexcept {
    return is_infinite_ || clock_type::now() <= ExpireTime();
  }

  bool operator==(const TtlValue& other) const {
    if(is_infinite_ && other.is_infinite_) {
      return true;
    }
    if(is_infinite_ || other.is_infinite_) {
      return false;
    }
    return ExpireTime() == other.ExpireTime();
  }

  std::weak_ordering operator<=>(const TtlValue& other) const {
    if(is_infinite_ && other.is_infinite_) {
      return std::weak_ordering::equivalent;
    }
    if(is_infinite_) {
      return std::weak_ordering::greater;
    }
    if(other.is_infinite_) {
      return std::weak_ordering::less;
    }
    return ExpireTime() <=> other.ExpireTime();
  }
};
}  // namespace avito_limiter
