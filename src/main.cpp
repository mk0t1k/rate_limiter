#include "lib/limiters.hpp"
#include "lib/mutex_storage.hpp"
#include "lib/sharded_storage.hpp"
#include "lib/meta.hpp"
#include "lib/ttl.hpp"
#include "lib/sliding_win_log.hpp"

#include <cassert>
#include <iostream>
#include <vector>

int main() {
  avito_limiter::StoredData<avito_limiter::SlidingWindowAlgo> limiter{std::nullopt, 3U, 4.0F};
  std::vector<avito_limiter::key_type> keys = {"ab", "cd", "ef"};
  //avito_limiter::MutexStorage<avito_limiter::TokenBucketAlgo, 
  //  avito_limiter::key_type> stor{keys.begin(), keys.end(), 
  //    std::tuple<std::size_t, float>{3Z, 4.0F}, std::nullopt};
  avito_limiter::IRateLimiter* intfc = new avito_limiter::MutexWinLimiter{
    keys.begin(), keys.end(), std::tuple<std::size_t, float>{3Z, 4.0F}
    , 4.0, 1.0};
  while (true) {
    std::string key_name;
    std::cin >> key_name;
    if(key_name == "exit") {
      break;
    }
    std::cout << intfc->Exists(key_name) << " " << intfc->Access(key_name) 
      << " " << intfc->GetNumAvail(key_name) << "\n";
  }
  delete intfc;
}