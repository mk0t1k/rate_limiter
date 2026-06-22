#pragma once

#include <chrono>

namespace avito_tests {
   
template<
  template<typename, typename> typename Base, 
  typename Clock = std::chrono::steady_clock
>
class MockStoredData final: public Base<MockStoredData<Base>, Clock> {
public:
  using clock_type = Clock;

  MockStoredData() = default;

  template<typename ... Args>
  MockStoredData(Args&&... args): 
    Base<MockStoredData, Clock>{std::forward<Args>(args)...} {}

  bool IsExpired() const noexcept {
    return false;
  }

  void Prolong() const noexcept {}
};
} // namespace avito_tests