#pragma once

#include <chrono>
#include <compare>

namespace avito_limiter {

class TtlValue final {
public:
  using clock_type = typename std::chrono::steady_clock;
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

  explicit TtlValue(value_type time_sec);

  void Set(value_type time_sec) noexcept;

  time_point ExpireTime() const noexcept;

  value_type GetTtl() noexcept;

  operator bool() const noexcept;

  bool operator==(const TtlValue& other) const;

  std::weak_ordering operator<=>(const TtlValue& other) const;
};
}  // namespace avito_limiter
