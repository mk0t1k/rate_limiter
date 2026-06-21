#pragma once

#include <cstddef>

#include <mutex>
#include <string>
#include <variant>

namespace avito_limiter {

using key_type = std::string;

class IRateLimiter {
public:
  virtual bool Access(const key_type& key) = 0;

  virtual std::size_t GetNumAvail(
    const key_type& key) const noexcept = 0;

  virtual ~IRateLimiter() = default;
};

namespace requirements {

template<typename T>
concept RateLimiterLogic = std::semiregular<T> && requires(T a, const T b) {
  {a.Access()} -> std::convertible_to<bool>;
  {b.GetNumAvail()} noexcept -> std::convertible_to<std::size_t>;
};

template<typename T>
concept RateLimiter = requires(T a, const T b, key_type key) {
  std::is_default_constructible_v<T>;
  {a.Access(key)} -> std::convertible_to<bool>;
  {b.GetNumAvail(key)} noexcept -> std::convertible_to<std::size_t>;
};

template<typename C, typename A>
concept Storage = requires(C a, const C b, key_type k) {
  std::is_default_constructible_v<C>;
  a.Visit(k, [](A&) -> int {return 0;});
  b.Visit(k, [](const A&) -> int {return 0;});
};

template<typename T>
concept PolyRateLimiter = requires() {
  std::is_base_of_v<IRateLimiter, T>;
  std::is_default_constructible_v<T>;
};
} // namespace requirements

template<
  template<requirements::RateLimiterLogic, typename ...> typename Container, 
  requirements::RateLimiterLogic Alg, 
  typename ... Aargs
>
requires requirements::Storage<Container<Alg, Aargs...>, Alg>
class RateLimiterWrapper final : public IRateLimiter {
  Container<Alg, Aargs...> storage_;
public:
  template<typename ... Args>
  RateLimiterWrapper(Args&&... args) : storage_{std::forward<Args>(args)...} {}

  bool Access(const key_type& key) {
    auto res = storage_.Visit(key, [](Alg& alg) {return alg.Access();});
    if(std::holds_alternative<std::false_type>(res)) {
      return false;
    }
    return std::get<bool>(res);
  }

  std::size_t GetNumAvail(const key_type& key) const noexcept {
    auto res = storage_.Visit(key, [](const Alg& alg) {return alg.GetNumAvail();});
    if(std::holds_alternative<std::false_type>(res)) {
      return 0UZ;
    }
    return std::get<std::size_t>(res);
  }
};

class Factory final {
public:
  template<requirements::PolyRateLimiter T, typename ... Args> 
  IRateLimiter* Allocate(Args&&... args) {
    return new T{std::forward<Args>(args)...};
  }
};
} // namespace avito_limiter