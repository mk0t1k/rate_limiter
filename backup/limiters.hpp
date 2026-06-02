#pragma once

#include <cstddef>

#include <tuple>

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

template<std::size_t ShardCapacity>
using ShardedWinLimiter = 
  RateLimiterWrapper<ShardedWrapper, SlidingWindowAlgo, 
  std::integral_constant<std::size_t, ShardCapacity>, key_type>;
} // namespace avito_limiter

/*namespace avito_meta {

template<avito_limiter::requirements::RateLimiterLogic Alg, typename SizeSeq>
struct GenerateShardedWrappersImpl {};

template<avito_limiter::requirements::RateLimiterLogic Alg>
struct GenerateShardedWrappersImpl<Alg, avito_meta::SizeSequence<>> {
  using type = avito_meta::AmorphicTuple<>;
};

template<avito_limiter::requirements::RateLimiterLogic Alg, 
  std::size_t Curr, std::size_t ... Others>
struct GenerateShardedWrappersImpl<Alg, avito_meta::SizeSequence<Curr, Others...>> {
  using curr_wrapper = avito_limiter::ShardedStorage<Curr, avito_limiter::key_type, Alg>;
  using prev = typename GenerateShardedWrappersImpl<
    Alg, avito_meta::SizeSequence<Others...>>::type;
  using type = AmorphPrependT<curr_wrapper, prev>;
};

template<avito_limiter::requirements::RateLimiterLogic Alg, 
  std::size_t Curr>
struct GenerateShardedWrappersImpl<Alg, avito_meta::SizeSequence<Curr>> {
  using curr_wrapper = avito_limiter::ShardedStorage<Curr, avito_limiter::key_type, Alg>;
  using type = AmorphicTuple<curr_wrapper>;
};

template<avito_limiter::requirements::RateLimiterLogic Alg, typename Sseq>
struct GenShardedForAlg {};

template<avito_limiter::requirements::RateLimiterLogic Alg>
struct GenShardedForAlg<Alg, SizeSequence<>> {
  using type = AmorphicTuple<>;
};

template<
  avito_limiter::requirements::RateLimiterLogic Alg, 
  std::size_t Curr, std::size_t ... Sizes>
struct GenShardedForAlg<Alg, SizeSequence<Curr, Sizes...>> {
  using type = GenerateShardedWrappersImpl<Alg, SizeSequence<Curr, Sizes...>>;
};

template<typename Algs, typename Caps>
struct GenShardedWrappers {};

template<
  avito_limiter::requirements::RateLimiterLogic Alg,
  avito_limiter::requirements::RateLimiterLogic ... Algs,
  std::size_t ... Capacities>
struct GenShardedWrappers<AmorphicTuple<Alg, Algs...>, SizeSequence<Capacities...>> {
  using prev = typename GenShardedWrappers<
    AmorphicTuple<Algs...>, 
    SizeSequence<Capacities...>>::type;
  using curr = typename GenShardedForAlg<Alg, SizeSequence<Capacities...>>::type;
  using type = AmorphConcatT<curr, prev>;
};

template<
  std::size_t ... Capacities>
struct GenShardedWrappers<AmorphicTuple<>, SizeSequence<Capacities...>> {
  using type = AmorphicTuple<>;
};
} // namespace avito_meta
*/
