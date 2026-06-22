#pragma once

#include <cstddef>

#include <future>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <variant>

namespace avito_limiter {

using key_type = std::string;

class IRateLimiter {
public:
  virtual bool Exists(const key_type& key) const noexcept = 0;

  virtual bool Access(const key_type& key) = 0;

  virtual std::size_t GetNumAvail(
    const key_type& key) const noexcept = 0;

  virtual ~IRateLimiter() = default;
};

class IRateShaper {
public:
  using future_type = std::future<bool>;

  virtual std::optional<future_type> AddRequest(const key_type& key) = 0;

  virtual std::size_t GetNumAvail() const noexcept = 0;

  virtual ~IRateShaper() = default;
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

template<typename C>
concept Storage = requires(C a, const C b, key_type k) {
  std::is_default_constructible_v<C>;
  a.Visit(k, [](auto&&) -> int {return 0;});
  b.Visit(k, [](auto&&) -> int {return 0;});
};

template<typename T>
concept PolyRateLimiter = requires() {
  std::is_base_of_v<IRateLimiter, T>;
  std::is_default_constructible_v<T>;
};
} // namespace requirements

template<
  template<template<typename, typename> typename, typename ...> typename Container, 
  template<typename, typename> typename Alg, 
  typename ... Aargs
>
requires requirements::Storage<Container<Alg, Aargs...>>
class RateLimiterWrapper final : public IRateLimiter {
  Container<Alg, Aargs...> storage_;
public:
  template<typename ... Args>
  RateLimiterWrapper(Args&&... args) : storage_{std::forward<Args>(args)...} {}

  bool Exists(const key_type& key) const noexcept override { 
    auto res = storage_.Visit(key, [](auto&& alg) { return 0; });
    if(std::holds_alternative<std::false_type>(res)) {
      return false;
    }
    return true;
  }

  bool Access(const key_type& key) {
    auto res = storage_.Visit(key, [](auto&& alg) {return alg.Access();});
    if(std::holds_alternative<std::false_type>(res)) {
      return false;
    }
    return std::get<bool>(res);
  }

  std::size_t GetNumAvail(const key_type& key) const noexcept {
    auto res = storage_.Visit(key, [](auto&& alg) {return alg.GetNumAvail();});
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