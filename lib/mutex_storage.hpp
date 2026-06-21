#pragma once

#include <cstddef>

#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

#include "interface.hpp"
#include "ttl.hpp"

namespace avito_limiter {

template<requirements::RateLimiterLogic Alg, typename KeyType>
class MutexStorage final {
public:
  using TtlValue_t = TtlValue<typename Alg::clock_type>;
  using ttl_dur_t = typename TtlValue_t::value_type;
private:
  struct Data {
    mutable std::mutex mtx;
    Alg rate_limiter;

    Data(const Alg& alg) : rate_limiter{alg} {}
  };

  using stored_type = std::unordered_map<KeyType, Data>;
  stored_type keys_;
  using iter_type = typename stored_type::iterator;

public:

  MutexStorage() = default;

  template<std::input_iterator InputIt, typename Sentinel>
  MutexStorage(InputIt begin, Sentinel end) {
    while (begin != end) {
      keys_.try_emplace(*begin, Alg{});
      ++begin;
    }
  }

  template<std::input_iterator InputIt, typename Sentinel>
  MutexStorage(InputIt begin, Sentinel end, const Alg& init_alg) {
    while (begin != end) {
      keys_.try_emplace(*begin, init_alg);
      ++begin;
    }
  }

  template<typename Self, typename Func, typename ... Args>
  auto Visit(this Self&& self, const KeyType& key, Func&& func, 
    Args&&... args) -> std::variant<std::false_type, 
    std::invoke_result_t<Func, Alg&, Args...>> {
    auto it = self.keys_.find(key);
    if(it != self.keys_.end()) {
      std::lock_guard lock(it->second.mtx);
      return std::forward<Func>(func)(it->second.rate_limiter, std::forward<Args>(args)...);
    }
    return std::false_type{};
  }
};
} // namespace avito_limiter
