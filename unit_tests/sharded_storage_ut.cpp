
#include <cstddef>
#include <new>
#include <thread>
#include <type_traits>

#include <gtest/gtest.h>

#include "lib/interface.hpp"
#include "lib/sliding_win_log.hpp"
#include <lib/sharded_storage.hpp>
#include <vector>

#include "mock_clock.hpp"

namespace al = avito_limiter;

using Alg = al::SlidingWindowAlgo<MockClock>;

struct TrackingAlg {
    static inline int alive = 0;
    static inline int destr_calls = 0;
    static inline int constr_calls = 0;

    TrackingAlg() { ++constr_calls, ++alive; }
    TrackingAlg(const TrackingAlg&) { ++constr_calls, ++alive; }
    ~TrackingAlg() { ++destr_calls, --alive; }

    bool Access() { return true; }
    std::size_t GetNumAvail() const noexcept { return 1; }
    bool Verify() { return true; }
    std::size_t GetAvail() const noexcept { return 1; }
};

class ShardedStorageTests : public ::testing::Test {
protected:

    void TearDown() override {
        MockClock::reset();
    }

    ShardedStorageTests() : 
        cont(keys.begin(), keys.end(), alg) {}
    
    Alg alg{3U, 4.0F, 3.0};
    std::vector<al::key_type> keys = {"a", "b", "c", "d", "e", "f", "g"};
    al::ShardedStorage<
        Alg, 
        std::integral_constant<size_t, 3>, 
        std::bool_constant<true>, 
        avito_limiter::key_type
    > cont;
};

TEST_F(ShardedStorageTests, ConcurrencyTest) {

    std::atomic<size_t> success_count{0};
    std::vector<std::jthread> ths;

    size_t num_threads = std::max(1u, std::thread::hardware_concurrency());
    size_t iterations_per_thread = 50'000; 

    al::key_type target_key = "a";

    for (size_t t = 0; t < num_threads; ++t) {
        ths.emplace_back([&]() { 
            for (size_t i = 0; i < iterations_per_thread; ++i) {
                auto res = cont.Visit(target_key, [](Alg& algo) {
                    return algo.Access();    
                });
                
                if (std::holds_alternative<bool>(res) && std::get<bool>(res)) {
                    success_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    ths.clear();

    EXPECT_EQ(success_count.load(), 3);
}

TEST_F(ShardedStorageTests, MemoryLeakTest) {
    {
        std::vector<al::key_type> keys = {"a", "b", "c", "d", "e", "f", "g"};
        al::ShardedStorage<
            TrackingAlg, 
            std::integral_constant<size_t, 3>, 
            std::bool_constant<true>, 
            avito_limiter::key_type
        > cont (keys.begin(), keys.end());
    }
    EXPECT_EQ(TrackingAlg::constr_calls, TrackingAlg::destr_calls);
}

TEST_F(ShardedStorageTests, AlingmentTest) {
    for (auto&& key : keys) {
        auto ptr = std::get<Alg*>(cont.Visit(key, [](Alg& alg){return &alg;}));
        EXPECT_TRUE(reinterpret_cast<size_t>(ptr) % std::hardware_constructive_interference_size == 0);
    }
}

TEST_F(ShardedStorageTests, TtlCleanerTest) {
    
}