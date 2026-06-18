#include <benchmark/benchmark.h>

#include "token_bucket.hpp"

static void BM_TokenBucketAccessAccepted(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    avito_limiter::TokenBucketAlgo bucket{1.0F, 1U};
    state.ResumeTiming();
    benchmark::DoNotOptimize(bucket.Access());
  }
}
BENCHMARK(BM_TokenBucketAccessAccepted);

static void BM_TokenBucketAccessRejected(benchmark::State& state) {
  avito_limiter::TokenBucketAlgo bucket{1.0F, 0U};
  for (auto _ : state) {
    benchmark::DoNotOptimize(bucket.Access());
  }
}
BENCHMARK(BM_TokenBucketAccessRejected);

static void BM_TokenBucketGetNumAvail(benchmark::State& state) {
  avito_limiter::TokenBucketAlgo bucket{1000.0F, 500U};
  for (auto _ : state) {
    benchmark::DoNotOptimize(bucket.GetNumAvail());
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_TokenBucketGetNumAvail);
