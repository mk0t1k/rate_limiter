#include <cstddef>
#include <new>
#include <thread>
#include <type_traits>
#include <vector>
#include <tuple>

#include <gtest/gtest.h>

#include "lib/interface.hpp"
#include "lib/mutex_storage.hpp"
#include "lib/sliding_win_log.hpp"
#include <lib/sharded_storage.hpp>

#include "mock_clock.hpp"

namespace al = avito_limiter;

class MutexStorageTests : public ::testing::Test {
protected:

    void TearDown() override {
        MockClock::reset();
    }

    MutexStorageTests() : 
        cont(keys.begin(), keys.end(), std::make_tuple(3U, 4.0F), 3.0) {}
    
    std::vector<al::key_type> keys = {"a", "b", "c", "d", "e", "f", "g"};
    
    al::MutexStorage<
        al::SlidingWindowAlgo, 
        avito_limiter::key_type,
        MockClock
    > cont;
};

TEST_F(MutexStorageTests, ConcurrencyTest) {

    std::atomic<size_t> success_count{0};
    std::vector<std::jthread> ths;

    size_t num_threads = std::max(1u, std::thread::hardware_concurrency());
    size_t iterations_per_thread = 50'000; 

    al::key_type target_key = "a";

    for (size_t t = 0; t < num_threads; ++t) {
        ths.emplace_back([&]() { 
            for (size_t i = 0; i < iterations_per_thread; ++i) {
                auto res = cont.Visit(target_key, [](auto& algo) {
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