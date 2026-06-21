#include "ttl.hpp"

#include <chrono>
#include <compare>
#include <optional>

namespace avito_limiter {

TtlValue::time_point TtlValue::ExpireTime() const noexcept {
  auto res = time_created_ + std::chrono::duration<offset_type>{ttl_sec_};
  time_point target_time = 
        std::chrono::time_point_cast<typename clock_type::duration>(
            res
        );
  return target_time;
}

TtlValue::value_type TtlValue::GetTtl() noexcept {
  if(is_infinite_) {
    return std::nullopt;
  }
  auto dur = ExpireTime() - clock_type::now();
  auto dur_sec = 
    std::chrono::duration_cast<std::chrono::duration<offset_type>>(dur);
  return dur_sec.count();
}

TtlValue::operator bool() const noexcept {
  return is_infinite_ || clock_type::now() <= ExpireTime();
}

bool TtlValue::operator==(const TtlValue& other) const {
  if(is_infinite_ && other.is_infinite_) {
    return true;
  }
  if(is_infinite_ || other.is_infinite_) {
    return false;
  }
  return ExpireTime() == other.ExpireTime();
}

std::weak_ordering TtlValue::operator<=>(const TtlValue& other) const {
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

TtlValue::TtlValue(value_type time_sec) {
  Set(time_sec);
}

void TtlValue::Set(value_type time_sec) noexcept {
  time_created_ = clock_type::now();
  if(time_sec) {
    ttl_sec_ = time_sec.value();
    is_infinite_ = false;    
  } else {
    is_infinite_ = true;
  }
}
} // namespace avito_limiter
