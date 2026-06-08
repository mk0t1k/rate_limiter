#include "limiters.hpp"
#include "mutex_storage.hpp"

#include <iostream>
#include <vector>

int main() {
  avito_limiter::SlidingWindowAlgo limiter{3U, 4.0F};
  std::vector<avito_limiter::key_type> keys = {"ab", "cd", "ef"};
  avito_limiter::MutexWinLimiter token_limiter{keys.begin(), 
    keys.end(), limiter};
  while (true) {
    std::string key_name;
    std::cin >> key_name;
    std::cout << token_limiter.Access(key_name) 
      << " " << token_limiter.GetNumAvail(key_name) << "\n";
  }
  
}