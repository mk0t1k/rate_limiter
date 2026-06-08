#pragma once

#include "interface.hpp"
#include "mutex_storage.hpp"
#include "sliding_win_log.hpp"
#include "token_bucket.hpp"

namespace avito_limiter {

using MutexTokenLimiter = MutexStorage<key_type, TokenBucketAlgo>;
using MutexWinLimiter = MutexStorage<key_type, SlidingWindowAlgo>;
} // namespace avito_limiter
