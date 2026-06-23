#pragma once

#include <cstddef>

#include <atomic>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

#include "algorithms/alg_base.hpp"
#include "interface.hpp"

namespace avito_limiter {

namespace impl {

struct EmptyMutex {};

template<bool>
struct MutexLock {
  MutexLock(EmptyMutex) {}
};

template<>
struct MutexLock<true> {
  std::lock_guard<std::mutex> lk;

  MutexLock(std::mutex& mtx) : lk{mtx} {}
};

template<bool>
struct MutexWrapper {
  EmptyMutex mtx;
};

template<>
struct MutexWrapper<true> {
  mutable std::mutex mtx;
};
} // namespace impl

template<
  template<typename, typename> typename Alg, typename KeyType,
  typename Clock = std::chrono::steady_clock, typename MutexPerKey = std::true_type
>
requires requirements::RateLimiterLogic<StoredData<Alg, Clock>>
class MutexStorage final {
  using stored_data_t = StoredData<Alg, Clock>;
  using time_point_t = decltype(Clock::now());
  using lock_type = impl::MutexLock<MutexPerKey::value>;
public:
  using lazy_counter_t = std::size_t;
  
  using time_offs_t = typename stored_data_t::value_type;
  using time_val_t = typename time_offs_t::value_type;
private:
  template<bool UseMutex>
  struct Data : impl::MutexWrapper<UseMutex> {
    stored_data_t rate_limiter;

    template<typename ... Types, std::size_t ... Nums>
    stored_data_t CreateLimiterHelper(
      time_offs_t offs, const std::tuple<Types...>& alg_args, 
      std::index_sequence<Nums...> seq
    ) {
      return stored_data_t{offs, std::get<Nums>(alg_args)...};
    }
    
    template<typename Sdata>
    Data(Sdata&& dt) : rate_limiter{std::forward<Sdata>(dt)} {}

    template<typename ... Types>
    Data(
      time_offs_t offs, const std::tuple<Types...>& alg_args) : 
      rate_limiter{CreateLimiterHelper(offs, alg_args, 
        std::make_index_sequence<sizeof...(Types)>{})} {}
  };

  class LazyState final {
    std::chrono::duration<double> dur_clean_;
    mutable std::atomic_bool notified_;
    time_point_t last_clean_;
  public:
    LazyState() = default;

    LazyState(double dur) : dur_clean_{dur}, last_clean_{Clock::now()} {}

    bool ShouldClean() const noexcept {
      auto threshold = last_clean_ + dur_clean_;
      if(Clock::now() < 
        std::chrono::time_point_cast<typename Clock::duration>(threshold)) {
        return false;
      }
      if(!notified_.load()) {
        notified_.store(true);
        return true;
      }
      
      return false;
    }

    void Update() noexcept {
      notified_.store(false);
      last_clean_ = Clock::now();
    }
  };

  using data_container_t = Data<MutexPerKey::value>;
  using stored_type = std::unordered_map<KeyType, data_container_t>;
  stored_type keys_;
  using iter_type = typename stored_type::iterator;
  mutable std::shared_mutex main_mutex_;
  std::optional<std::thread> cleaner_thread_;
  std::optional<std::atomic_bool> cleaner_run_;
  std::optional<LazyState> lazy_state_;
  Data<false> default_algo_;

  void DoCleanup() {
    auto curr_it = keys_.begin();
    while (curr_it != keys_.end()) {
      decltype(curr_it) next = curr_it;
      ++next;
      if(curr_it->second.rate_limiter.IsExpired()) {
        keys_.erase(curr_it);
      }
      curr_it = next;
    } 
  }

  void RunCleaner(double wait_sec) {
    while (cleaner_run_.value().load(std::memory_order_relaxed)) {
      std::this_thread::sleep_for(std::chrono::duration<double>{wait_sec});
      std::unique_lock u_lk(main_mutex_);
      DoCleanup();
    }
  }

  void InitCleaner(time_offs_t update_dur, bool is_lazy) {
    if(!update_dur) {
      return;
    }
    if(!is_lazy) {
      cleaner_run_.emplace(true);
      cleaner_thread_.emplace(&MutexStorage::RunCleaner, this, 
        update_dur.value());
    } else {
      lazy_state_.emplace(update_dur.value());
    }
  }

public:

  MutexStorage() = default;

  template<std::input_iterator InputIt, typename Sentinel>
  MutexStorage(InputIt begin, Sentinel end, 
    time_offs_t key_prolong = std::nullopt, 
    time_offs_t update_dur = std::nullopt, bool is_lazy=false) : 
    default_algo_{key_prolong, std::tuple<>{}} { 
    while (begin != end) {
      keys_.try_emplace(*begin, key_prolong, std::tuple<>{});
      ++begin;
    }
    InitCleaner(update_dur, is_lazy);
  }

  template<std::input_iterator InputIt, typename Sentinel, typename ... AlgTypes>
  MutexStorage(InputIt begin, Sentinel end, 
    const std::tuple<AlgTypes...>& alg_args, 
    time_offs_t key_prolong = std::nullopt,
    time_offs_t update_dur = std::nullopt, bool is_lazy=false) : 
    default_algo_{key_prolong, alg_args} {
    while (begin != end) {
      keys_.try_emplace(*begin, key_prolong, alg_args);
      ++begin;
    }
    InitCleaner(update_dur, is_lazy);
  }

  MutexStorage(const MutexStorage& other) = delete;

  MutexStorage& operator=(const MutexStorage& other) = delete;

  bool Add(const KeyType& key, time_offs_t key_ttl = std::nullopt) {
    std::unique_lock lk(main_mutex_);
    auto tmp = default_algo_.rate_limiter;
    tmp.SetTtl(key_ttl);
    auto res = keys_.try_emplace(key, std::move(tmp));
    return res.second;
  }

  template<typename ... Types>
  bool Emplace(time_offs_t key_ttl, const KeyType& key, 
    const std::tuple<Types...>& alg_args) {
    std::unique_lock lk(main_mutex_);
    auto res = keys_.try_emplace(key, key_ttl, alg_args);
    return res.second;
  }

  template<typename Self, typename Func, typename ... Args>
  auto Visit(this Self&& self, const KeyType& key, Func&& func, 
    Args&&... args) -> std::variant<std::false_type, 
    std::invoke_result_t<Func, stored_data_t&, Args...>> {
    using self_type = decltype(self);
    using raw_self_type = std::remove_reference_t<self_type>;
    if constexpr (!std::is_const_v<raw_self_type>) {
      if(self.lazy_state_ && self.lazy_state_.value().ShouldClean()) {
        std::unique_lock ul(self.main_mutex_);
        self.DoCleanup();
        self.lazy_state_.value().Update();
      }
    }
    using main_lk_type = std::conditional_t<MutexPerKey::value, 
      std::shared_lock<std::shared_mutex>, std::unique_lock<std::shared_mutex>>;
    main_lk_type main_lock(self.main_mutex_);
    auto it = self.keys_.find(key);
    if(it != self.keys_.end()) {
      lock_type lock(it->second.mtx);
      return std::forward<Func>(func)(it->second.rate_limiter, std::forward<Args>(args)...);
    }
    return std::false_type{};
  }

  ~MutexStorage() {
    if(cleaner_thread_) {
      cleaner_run_.value().store(false, std::memory_order_relaxed);
      cleaner_thread_.value().join();
    }
  }
};
} // namespace avito_limiter
