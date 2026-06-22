#pragma once

#include <chrono>
#include <cstddef>

#include "ttl.hpp"

namespace avito_limiter {

template<
  template<typename, typename> typename Base, 
  typename Clock = std::chrono::steady_clock
>
class StoredData final: public Base<StoredData<Base, Clock>, Clock> {
public:
  using clock_type = Clock;
  using TtlValue_t = TtlValue<clock_type>;
  using value_type = typename TtlValue_t::value_type;
private:
  mutable TtlValue_t ttl_;
  value_type prolong_sec_;

public:
  StoredData() = default;

  template<typename ... Args>
  StoredData(value_type val, Args&&... args): 
    Base<StoredData, Clock>{std::forward<Args>(args)...}, ttl_{val}, 
    prolong_sec_{val} {}

  bool IsExpired() const noexcept {
    return !static_cast<bool>(ttl_);
  }

  void Prolong() const noexcept {
    ttl_.Set(prolong_sec_);
  }
};
} // namespace avito_limiter
