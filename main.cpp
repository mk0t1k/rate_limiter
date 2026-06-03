#include <cstddef>

#include <algorithm>
#include <chrono>
#include <concepts>
#include <iostream>
#include <string>
#include <type_traits>

namespace avito_limiter {

using key_type = std::string;

struct IrateLimiter {
  virtual void Update() noexcept = 0;

  virtual bool Receive(const key_type& key) = 0;
  
  virtual ~IrateLimiter() = default;
};

struct SlidingWindowLog : public IrateLimiter {
	void SetCapacity(std::size_t tgt);
	
	void SetTimeWidth(double cnt_sec);

  void Update() noexcept;

  bool Receive(const key_type& key);
};

struct TokenBucket : public IrateLimiter {
  using clock_type = std::chrono::steady_clock;
  using timepoint_type = decltype(clock_type::now());

  TokenBucket(std::size_t cap, std::size_t initial_cap, 
    double r_refill) : 
    capacity_{cap}, cr_num_tok_{std::min(capacity_, initial_cap)}, 
    refill_rt_{r_refill}, last_{clock_type::now()} {}

	void SetCapacity(std::size_t tgt) {
    capacity_ = tgt;
    if(cr_num_tok_ > capacity_) {
      cr_num_tok_ = capacity_;
    }
  }
	
	void SetRefillRate(double tk_per_sec) {
    refill_rt_ = tk_per_sec;
  }

  void Update() noexcept override {
    timepoint_type curr = clock_type::now();
    double elapsed_sec = std::chrono::duration<double>(curr - last_).count();
    cr_num_tok_ += std::size_t(elapsed_sec * refill_rt_);
    last_ = curr;
  }

  bool Receive(const key_type& key) override {
    if(cr_num_tok_) {
      --cr_num_tok_;
      return true;
    }
    return false;
  }
private:
  std::size_t capacity_ = 0U;
  double refill_rt_ = 0.0;
  std::size_t cr_num_tok_ = 0U;

  timepoint_type last_;
};
} // namespace avito_limiter

int main() {
  avito_limiter::TokenBucket buck{3U, 2U, 3.0};
  std::cout << buck.Receive("aboba") << "\n" 
    << buck.Receive("sussyt") << "\n"
    << buck.Receive("lolz") << "\n"
    << buck.Receive("lolz") << "\n";
}