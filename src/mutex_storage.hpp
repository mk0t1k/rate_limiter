#pragma once

#include <cstddef>

#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>

#include "interface.hpp"

namespace avito_limiter {

template<typename KeyType, RateLimAlgorithm Alg>
class MutexStorage : public IRateLimiter {
  struct Data {
    mutable std::mutex mtx;
    Alg rate_limiter;

    Data(const Alg& alg) : rate_limiter{alg} {}
  };

  using stored_type = std::unordered_map<KeyType, Data>;
  stored_type keys_;
  using iter_type = typename stored_type::iterator;

  template<typename Func, typename ... Args>
  auto Visit(const KeyType& key, Func&& func, 
    Args&&... args) -> std::variant<std::false_type, 
    std::invoke_result_t<Func, Alg&, Args...>> {
    auto it = keys_.find(key);
    if(it != keys_.end()) {
      std::lock_guard lock(it->second.mtx);
      return func(it->second.rate_limiter, args...);
    }
    return std::false_type{};
  }

  template<typename Func, typename ... Args>
  auto Visit(const KeyType& key, Func&& func, 
    Args&&... args) const -> std::variant<std::false_type, 
    std::invoke_result_t<Func, Alg&, Args...>> {
    auto it = keys_.find(key);
    if(it != keys_.end()) {
      std::lock_guard lock(it->second.mtx);
      return func(it->second.rate_limiter, args...);
    }
    return std::false_type{};
  }

public:
  MutexStorage() = default;

  template<std::input_iterator InputIt>
  MutexStorage(InputIt begin, InputIt end, Alg init_alg) {
    while (begin != end) {
      keys_.emplace(*begin, init_alg);
      ++begin;
    }
  }

  bool Access(const KeyType& key) {
    auto res = Visit(key, [](Alg& alg) {return alg.Access();});
    if(std::holds_alternative<std::false_type>(res)) {
      return false;
    }
    return std::get<bool>(res);
  }

  std::size_t GetNumAvail(const KeyType& key) const noexcept {
    auto res = Visit(key, [](const Alg& alg) {return alg.GetNumAvail();});
    if(std::holds_alternative<std::false_type>(res)) {
      return 0UZ;
    }
    return std::get<std::size_t>(res);
  }
};
} // namespace avito_limiter
