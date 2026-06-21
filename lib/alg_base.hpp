#pragma once

#include <cstddef>

#include "ttl.hpp"

namespace avito_limiter {

template<typename Derived>
class AlgBase {
public:
  using value_type = typename TtlValue::value_type;
private:
  mutable TtlValue ttl_;
  value_type prolong_sec_;
protected:

  AlgBase() = default;

  AlgBase(value_type val): ttl_{val}, prolong_sec_{val} {}

public:
  bool IsExpired() const noexcept {
    return !static_cast<bool>(ttl_);
  }

  bool Access() noexcept {
    if(IsExpired()) {
      return false;
    }
    ttl_.Set(prolong_sec_);
    return static_cast<Derived*>(this)->Verify();
  }

  std::size_t GetNumAvail() const noexcept {
    if(IsExpired()) {
      return 0Z;
    }
    return static_cast<const Derived*>(this)->GetAvail();
  }
};
} // namespace avito_limiter
