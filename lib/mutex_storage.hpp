#pragma once

#include <cstddef>

#include <atomic>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

#include "alg_base.hpp"
#include "interface.hpp"
#include "meta.hpp"
#include "ttl.hpp"

namespace avito_limiter {

template<
  template<typename, typename> typename Alg, typename KeyType,
  typename Clock = std::chrono::steady_clock
>
requires requirements::RateLimiterLogic<StoredData<Alg, Clock>>
class MutexStorage final {
  using stored_data_t = StoredData<Alg, Clock>;
  using time_point_t = decltype(Clock::now());
public:
  using lazy_counter_t = std::size_t;
  
  using time_offs_t = typename stored_data_t::value_type;
  using time_val_t = typename time_offs_t::value_type;
private:
  struct Data {
    mutable std::mutex mtx;
    StoredData<Alg, Clock> rate_limiter;

    template<typename ... Types, std::size_t ... Nums>
    StoredData<Alg, Clock> CreateLimiterHelper(
      time_offs_t offs, const std::tuple<Types...>& alg_args, 
      std::index_sequence<Nums...> seq
    ) {
      return StoredData<Alg, Clock>{offs, std::get<Nums>(alg_args)...};
    }
    
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
        std::chrono::duration_cast<typename Clock::duration>(threshold)) {
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

  using stored_type = std::unordered_map<KeyType, Data>;
  stored_type keys_;
  using iter_type = typename stored_type::iterator;
  mutable std::shared_mutex main_mutex_;
  std::optional<std::thread> cleaner_thread_;
  std::optional<std::atomic_bool> cleaner_run_;
  std::optional<LazyState> lazy_state_;

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
    time_offs_t update_dur = std::nullopt, bool is_lazy=false) { 
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
    time_offs_t update_dur = std::nullopt, bool is_lazy=false) {
    while (begin != end) {
      keys_.try_emplace(*begin, key_prolong, alg_args);
      ++begin;
    }
    InitCleaner(update_dur, is_lazy);
  }

  MutexStorage(const MutexStorage& other) = delete;

  MutexStorage& operator=(const MutexStorage& other) = delete;

  template<typename Self, typename Func, typename ... Args>
  auto Visit(this Self&& self, const KeyType& key, Func&& func, 
    Args&&... args) -> std::variant<std::false_type, 
    std::invoke_result_t<Func, stored_data_t&, Args...>> {
    std::shared_lock main_lock(self.main_mutex_);
    auto it = self.keys_.find(key);
    if(it != self.keys_.end()) {
      std::lock_guard lock(it->second.mtx);
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
