#include "lib/algorithms/limiters.hpp"
#include "lib/mutex_storage.hpp"
#include "lib/meta.hpp"
#include "lib/ttl.hpp"
#include "lib/algorithms/sliding_win_log.hpp"
#include "lib/algorithms/leaky_bucket.hpp"

#include <cassert>
#include <iostream>
#include <vector>

void PrintAccessInfo(const avito_limiter::key_type& key_name, 
  avito_limiter::IRateLimiter* intfc) {
  assert(intfc != nullptr);
  std::cout << "Access to key " << key_name << " " << intfc->Exists(key_name) 
    << " " << intfc->Access(key_name) << " " << intfc->GetNumAvail(key_name) << "\n";
}

int main() {
  using mutex_cont = avito_limiter::MutexStorage<
    avito_limiter::TokenBucketAlgo, avito_limiter::key_type>;
  using sharded_cont = avito_limiter::ShardedWrapper<
    avito_limiter::MutexTokenLimiter, avito_limiter::key_type, 4UZ>;

  std::vector<avito_limiter::key_type> keys = {"a", "cd", "ef"};
  avito_limiter::ShardedTokenLimiter<4U, true> c_sharded{keys.begin(), keys.end(), 
    std::tuple<std::size_t, float>{3Z, 2.0F}, 4.0, 1.0};
  //avito_limiter::MutexStorage<avito_limiter::TokenBucketAlgo, 
  //  avito_limiter::key_type> stor{keys.begin(), keys.end(), 
  //    std::tuple<std::size_t, float>{3Z, 4.0F}, std::nullopt};
  avito_limiter::IRateShaper* shaper = new avito_limiter::LeakyBucketShaper{
    3, 2, 1.0
  };
  
  avito_limiter::IRateLimiter* intfc = new avito_limiter::MutexTokenLimiter{
    keys.begin(), keys.end(), std::tuple<std::size_t, float>{3Z, 2.0F}, 4.0, 1.0};
  while (true) {
    std::string key_name;
    std::cin >> key_name;
    if(key_name == "exit") {
      break;
    }
    PrintAccessInfo(key_name, intfc);
    PrintAccessInfo(key_name, static_cast<avito_limiter::IRateLimiter*>(&c_sharded));
    std::cout << static_cast<bool>(shaper->AddRequest(key_name)) 
      << " " << shaper->GetNumAvail() << "\n";
  }
  delete intfc;
  delete shaper;
}