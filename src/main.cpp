#include <cstddef>

#include <atomic>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include "interface.hpp"
#include "token_bucket.hpp"

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