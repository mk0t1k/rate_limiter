#include <benchmark/benchmark.h>

#include "token_bucket.hpp"

namespace {

const avito_limiter::key_type kKey{"request-id"};

}  // namespace

static void BM_TokenBucketReceiveAccepted(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    avito_limiter::TokenBucket bucket{1U, 1U, 0.0};
    state.ResumeTiming();
    benchmark::DoNotOptimize(bucket.Receive(kKey));
  }
}
BENCHMARK(BM_TokenBucketReceiveAccepted);

static void BM_TokenBucketReceiveRejected(benchmark::State& state) {
  avito_limiter::TokenBucket bucket{1U, 0U, 0.0};
  for (auto _ : state) {
    benchmark::DoNotOptimize(bucket.Receive(kKey));
  }
}
BENCHMARK(BM_TokenBucketReceiveRejected);

static void BM_TokenBucketUpdate(benchmark::State& state) {
  avito_limiter::TokenBucket bucket{1000U, 500U, 1000.0};
  for (auto _ : state) {
    bucket.Update();
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_TokenBucketUpdate);
