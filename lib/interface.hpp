#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <concepts>
#include <future>
#include <mutex>
#include <new>
#include <optional>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "meta.hpp"

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

template<typename T, typename K>
concept HashFunc = std::semiregular<T> && requires(const T a, const K b) {
  {a(b)} noexcept -> std::convertible_to<std::uint64_t>;
};

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
  using stored_type = Container<Alg, Aargs...>;
  Container<Alg, Aargs...> storage_;
public:
  using time_offs_t = typename stored_type::time_offs_t;

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

namespace impl {

constexpr std::size_t kCacheLineSize = std::hardware_destructive_interference_size;
} // namespace impl

template<
  typename Limiter, typename KeyType, std::size_t CountShards, 
  typename Hash=std::hash<KeyType>, bool OptAlign=true
>
requires requirements::HashFunc<Hash, KeyType>
class ShardedWrapper final : public IRateLimiter {
  static_assert(CountShards);

  static constexpr std::size_t kAlignment = avito_meta::ValueConditional<
    std::size_t,
    OptAlign,
    avito_meta::Lcm<impl::kCacheLineSize, alignof(Limiter)>::value,
    alignof(Limiter)>::value;

  struct alignas(kAlignment) LimiterPlace {
    alignas(Limiter) std::byte bytes[sizeof(Limiter)];
  };

  std::size_t GetKeyIndex(const KeyType& key) const noexcept {
    return hash_func_(key) % CountShards;
  }

  // Pre: !created(limiters_[idx]) && idx < CountShards
  template<typename ... Args>
  Limiter* EmplaceAt(std::size_t idx, Args&&... args) {
    Limiter* ptr = new (limiters_[idx].bytes) Limiter{std::forward<Args>(args)...};
    return ptr;
  }

  void DestroyAt(std::size_t idx) {
    std::byte* byte_ptr = limiters_[idx].bytes;
    Limiter* lim_ptr = reinterpret_cast<Limiter*>(byte_ptr);
    lim_ptr->~Limiter();
  }

  Limiter* GetByKey(const KeyType& key) noexcept {
    std::size_t c_idx = GetKeyIndex(key);
    std::byte* byte_ptr = limiters_[c_idx].bytes;
    Limiter* lim_ptr = reinterpret_cast<Limiter*>(byte_ptr);
    return lim_ptr;
  }

  const Limiter* GetByKey(const KeyType& key) const noexcept {
    std::size_t c_idx = GetKeyIndex(key);
    const std::byte* byte_ptr = limiters_[c_idx].bytes;
    const Limiter* lim_ptr = reinterpret_cast<const Limiter*>(byte_ptr);
    return lim_ptr;
  }

  Hash hash_func_;
  LimiterPlace limiters_[CountShards];
public:
  using hasher = Hash;
  
  template<std::input_iterator InputIt, typename Sentinel, 
    typename ... Args>
  ShardedWrapper(InputIt begin, Sentinel end, Args&&... args) {
    std::array<std::vector<KeyType>, CountShards> keys_per_limiter;
    while (begin != end) {
      KeyType curr{*begin};
      std::size_t curr_idx = GetKeyIndex(curr);
      keys_per_limiter[curr_idx].push_back(std::move(curr));
      ++begin;
    }
    for(std::size_t i = 0; i < CountShards; ++i) {
      EmplaceAt(i, keys_per_limiter[i].begin(), keys_per_limiter[i].end(),
        std::forward<Args>(args)...);
    }
  }

  bool Exists(const key_type& key) const noexcept override {
    return GetByKey(key)->Exists(key);
  }

  bool Access(const key_type& key) override {
    return GetByKey(key)->Access(key);
  }

  std::size_t GetNumAvail(const key_type& key) const noexcept override {
    return GetByKey(key)->GetNumAvail(key);
  }

  ~ShardedWrapper() {
    for(std::size_t i = 0UZ; i < CountShards; ++i) {
      DestroyAt(i);
    }
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