#include "lib/limiters.hpp"
#include "lib/mutex_storage.hpp"
#include "lib/sharded_storage.hpp"
#include "lib/meta.hpp"
#include "lib/ttl.hpp"
#include "lib/sliding_win_log.hpp"
#include "lib/leaky_bucket.hpp"

#include <cassert>
#include <iostream>
#include <vector>


int main() {
  std::vector<avito_limiter::key_type> keys = {"a", "cd", "ef"};
  //avito_limiter::MutexStorage<avito_limiter::TokenBucketAlgo, 
  //  avito_limiter::key_type> stor{keys.begin(), keys.end(), 
  //    std::tuple<std::size_t, float>{3Z, 4.0F}, std::nullopt};
  avito_limiter::IRateShaper* shaper = new avito_limiter::LeakyBucketShaper{
    3, 2, 1.0
  };
  
  avito_limiter::IRateLimiter* intfc = new avito_limiter::MutexTokenLimiter{
    keys.begin(), keys.end(), std::tuple<std::size_t, float>{3Z, 2.0F}};
  while (true) {
    std::string key_name;
    std::cin >> key_name;
    if(key_name == "exit") {
      break;
    }
    std::cout << intfc->Exists(key_name) << " " << intfc->Access(key_name) 
      << " " << intfc->GetNumAvail(key_name) << " " << 
      static_cast<bool>(shaper->AddRequest(key_name)) << " " << shaper->GetNumAvail() << "\n";
  }
  delete intfc;
  delete shaper;
}