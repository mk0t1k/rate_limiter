#include "lib/limiters.hpp"
#include "lib/mutex_storage.hpp"
#include "lib/sharded_storage.hpp"
#include "lib/meta.hpp"

#include <iostream>
#include <vector>

int main() {
  avito_limiter::SlidingWindowAlgo limiter{3U, 4.0F};
  std::vector<avito_limiter::key_type> keys = {"ab", "cd", "ef"};
  avito_limiter::IRateLimiter* intfc = new avito_limiter::ShardedWinLimiter<3U, true>{
    keys.begin(), keys.end(), limiter};
  //avito_limiter::ShardedStorage<3, 
  //  avito_limiter::key_type, avito_limiter::SlidingWindowAlgo> s_sharded{
  //  keys.begin(), keys.end(), limiter};
  
  while (true) {
    std::string key_name;
    std::cin >> key_name;
    if(key_name == "exit") {
      break;
    }
    std::cout << intfc->Access(key_name) 
      << " " << intfc->GetNumAvail(key_name) << "\n";
  }
  delete intfc;
}