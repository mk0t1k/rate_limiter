#include <cstddef>

#include <atomic>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>

namespace avito_limiter {

using key_type = std::string;

class IrateLimiter {
public:
  std::size_t GetQueryLimit() const noexcept {
    return curr_limit_;
  }

  virtual void Update() noexcept = 0;

  virtual bool Receive(const key_type& key) = 0;
  
  virtual ~IrateLimiter() = default;
protected:
  std::size_t curr_limit_ = 0U;
};

class SlidingWindowLog : public IrateLimiter {
public:
	void SetCapacity(std::size_t tgt);
	
	void SetTimeWidth(double cnt_sec);

  void Update() noexcept;

  bool Receive(const key_type& key);
};

class TokenBucket : public IrateLimiter {
public:
  using clock_type = std::chrono::steady_clock;
  using timepoint_type = decltype(clock_type::now());

  TokenBucket(std::size_t cap, std::size_t initial_cap, 
    double r_refill) :
    capacity_{cap}, 
    refill_rt_{r_refill}, last_{clock_type::now()} {
      curr_limit_ = std::min(capacity_, initial_cap);
  }

	void SetCapacity(std::size_t tgt) {
    std::lock_guard lock(intfc_mutex_);
    capacity_ = tgt;
    if(curr_limit_ > tgt) {
      curr_limit_ = tgt;
    }
  }
	
	void SetRefillRate(double tk_per_sec) {
    std::lock_guard lock(intfc_mutex_);
    refill_rt_ = tk_per_sec;
  }

  void Update() noexcept override {
    timepoint_type curr = clock_type::now();
    double elapsed_sec = std::chrono::duration<double>(curr - last_).count();
    curr_limit_ = std::min(
      curr_limit_ + std::size_t(elapsed_sec * refill_rt_), capacity_);
    last_ = curr;
  }

  bool Receive(const key_type& key) override {
    std::lock_guard lock(intfc_mutex_);
    if(curr_limit_) {
      --curr_limit_;
      return true;
    }
    return false;
  }
private:
  std::size_t capacity_ = 0U;
  double refill_rt_ = 0.0;
  std::mutex intfc_mutex_;

  timepoint_type last_;
};
} // namespace avito_limiter

int main() {
  avito_limiter::IrateLimiter* limiter = 
    new avito_limiter::TokenBucket{3U, 2U, 3.0};
  std::atomic<bool> work;
  work.store(true, std::memory_order_relaxed);
  std::thread update_thread{[limiter, &work]() {
    while (work.load(std::memory_order_relaxed)){
      limiter->Update();
      std::this_thread::sleep_for(std::chrono::milliseconds{1000});
    }
    std::cout << "Update thread finished\n";
  }};
  std::string curr;
  while (true) {
    if(curr == "exit") {
      break;
    }
    std::cout << limiter->GetQueryLimit() << "\n";
    std::cin >> curr;
    if(curr == "-") {
      continue;
    }
    if(limiter->Receive(curr)) {
      std::cout << "Ok\n";
    } else {
      std::cout << "Rate limit exceeded\n";
    }
  }
  work.store(false, std::memory_order_relaxed);
  update_thread.join();
  delete limiter;
}