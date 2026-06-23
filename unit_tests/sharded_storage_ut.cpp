#include <cstddef>
#include <new>
#include <thread>
#include <type_traits>
#include <vector>
#include <tuple>
#include <memory>

#include <gtest/gtest.h>

#include "lib/interface.hpp"
#include "lib/mutex_storage.hpp"
#include "lib/algorithms/sliding_win_log.hpp"

#include "mock_clock.hpp"

namespace al = avito_limiter;

struct TrackingLimiter : public al::IRateLimiter {
    static inline std::vector<void*> instances;
    static inline int alive = 0;
    static inline int destr_calls = 0;
    static inline int constr_calls = 0;

    static void Reset() {
        instances.clear();
        alive = 0;
        destr_calls = 0;
        constr_calls = 0;
    }

    template<typename ... Args>
    TrackingLimiter(Args&&...) {
        instances.push_back(this); 
        ++constr_calls;
        ++alive;
    }

    ~TrackingLimiter() override {
        ++destr_calls;
        --alive;
    }

    bool Exists(const al::key_type&) const noexcept override { return true; }
    bool Access(const al::key_type&) override { return true; }
    std::size_t GetNumAvail(const al::key_type&) const noexcept override { return 1; }
};

using TestMutexWinLimiter = 
  al::RateLimiterWrapper<al::MutexStorage, al::SlidingWindowAlgo, al::key_type, MockClock>;

template<std::size_t CountShards, bool OptAlign>
using TestShardedWinLimiter = 
  al::ShardedWrapper<TestMutexWinLimiter, al::key_type, CountShards, std::hash<al::key_type>, OptAlign>;


class ShardedWrapperTests : public ::testing::Test {
protected:
    void TearDown() override {
        MockClock::reset();
    }

    ShardedWrapperTests() : 
        cont(keys.begin(), keys.end(), std::make_tuple(3U, 4.0F), 3.0) {}
    
    std::vector<al::key_type> keys = {"a", "b", "c", "d", "e", "f", "g"};
    TestShardedWinLimiter<3, true> cont;
};

TEST_F(ShardedWrapperTests, ConcurrencyTest) {
    std::atomic<size_t> success_count{0};
    std::vector<std::jthread> ths;

    size_t num_threads = std::max(1u, std::thread::hardware_concurrency());
    size_t iterations_per_thread = 50'000; 

    al::key_type target_key = "a";

    for (size_t t = 0; t < num_threads; ++t) {
        ths.emplace_back([&]() { 
            for (size_t i = 0; i < iterations_per_thread; ++i) {
                if (cont.Access(target_key)) {
                    success_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    ths.clear();

    EXPECT_EQ(success_count.load(), 3);
}

TEST(ShardedWrapperSimpleTests, AlignmentAndMemoryLeakTest) {
    TrackingLimiter::Reset();
    {
        std::vector<al::key_type> keys = {"a", "b", "c", "d", "e", "f", "g"};
        
        al::ShardedWrapper<TrackingLimiter, al::key_type, 3, std::hash<al::key_type>, true> wrapper(
            keys.begin(), keys.end()
        );

        EXPECT_EQ(TrackingLimiter::constr_calls, 3);
        EXPECT_EQ(TrackingLimiter::alive, 3);

        for (void* ptr : TrackingLimiter::instances) {
            EXPECT_TRUE(reinterpret_cast<size_t>(ptr) % std::hardware_destructive_interference_size == 0);
        }
    }
    EXPECT_EQ(TrackingLimiter::destr_calls, 3);
    EXPECT_EQ(TrackingLimiter::alive, 0);
}