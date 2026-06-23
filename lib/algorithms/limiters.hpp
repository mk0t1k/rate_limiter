#pragma once

#include <cstddef>

#include <type_traits>
#include <utility>

#include "../interface.hpp"
#include "../meta.hpp"
#include "../mutex_storage.hpp"
#include "sliding_win_log.hpp"
#include "token_bucket.hpp"

namespace avito_limiter {

using MutexTokenLimiter = 
  RateLimiterWrapper<MutexStorage, TokenBucketAlgo, key_type>;

using MutexWinLimiter = 
  RateLimiterWrapper<MutexStorage, SlidingWindowAlgo, key_type>;

template<std::size_t ShardCapacity, bool OptAlign>
using ShardedWinLimiter = 
  ShardedWrapper<
    MutexWinLimiter, key_type, ShardCapacity, 
    std::hash<key_type>, OptAlign>;

template<std::size_t ShardCapacity, bool OptAlign>
using ShardedTokenLimiter = 
  ShardedWrapper<
    MutexTokenLimiter, key_type, ShardCapacity, 
    std::hash<key_type>, OptAlign>;
} // namespace avito_limiter
