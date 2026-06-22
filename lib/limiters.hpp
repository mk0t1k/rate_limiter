#pragma once

#include <cstddef>

#include <type_traits>

#include "interface.hpp"
#include "meta.hpp"
#include "mutex_storage.hpp"
#include "sharded_storage.hpp"
#include "sliding_win_log.hpp"
#include "token_bucket.hpp"

namespace avito_limiter {

using MutexTokenLimiter = 
  RateLimiterWrapper<MutexStorage, TokenBucketAlgo, key_type>;

using MutexWinLimiter = 
  RateLimiterWrapper<MutexStorage, SlidingWindowAlgo, key_type>;

//template<std::size_t ShardCapacity, bool OptAlign>
//using ShardedWinLimiter = 
//  RateLimiterWrapper<ShardedStorage, SlidingWindowAlgo<>, 
//  std::integral_constant<std::size_t, ShardCapacity>, 
//  std::integral_constant<bool, OptAlign>, key_type>;
} // namespace avito_limiter
