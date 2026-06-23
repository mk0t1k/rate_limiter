#include <cstddef>
#include <new>
#include <thread>
#include <type_traits>
#include <vector>
#include <tuple>
#include <variant>

#include <gtest/gtest.h>

#include "lib/interface.hpp"
#include "lib/mutex_storage.hpp"
#include "lib/algorithms/sliding_win_log.hpp"

#include "mock_clock.hpp"


using namespace std::chrono_literals;
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

TEST_F(MutexStorageTests, EmplaceAndSetTtlTest) {
    al::key_type new_key = "z";
    
    auto check_exist = cont.Visit(new_key, [](auto& algo) { return true; });
    EXPECT_TRUE(std::holds_alternative<std::false_type>(check_exist));

    cont.SetKeyTtl(10.0);

    bool emplaced = cont.Emplace(new_key, std::make_tuple(3U, 4.0F));
    EXPECT_TRUE(emplaced);

    auto check_avail = cont.Visit(new_key, [](auto& algo) { return algo.GetNumAvail(); });
    ASSERT_TRUE(std::holds_alternative<std::size_t>(check_avail));
    EXPECT_EQ(std::get<std::size_t>(check_avail), 3);

    al::key_type other_key = "y";
    bool emplaced_other = cont.Emplace(5.0, other_key, std::make_tuple(3U, 4.0F));
    EXPECT_TRUE(emplaced_other);
}

TEST(MutexStorageTestsSimple, LazyCleanupTest) {
    MockClock::reset();
    std::vector<al::key_type> keys = {"temp"};
    
    al::MutexStorage<al::SlidingWindowAlgo, al::key_type, MockClock> storage(
        keys.begin(), keys.end(), std::make_tuple(3U, 4.0F), 2.0, 5.0, true
    );

    auto res = storage.Visit("temp", [](auto& algo) { return algo.Access(); });
    EXPECT_TRUE(std::holds_alternative<bool>(res) && std::get<bool>(res));

    MockClock::advance(3s);

    auto res2 = storage.Visit("temp", [](auto& algo) { return true; });
    EXPECT_TRUE(std::holds_alternative<bool>(res2));

    MockClock::advance(3s);

    auto res3 = storage.Visit("temp", [](auto& algo) { return true; });
    EXPECT_TRUE(std::holds_alternative<std::false_type>(res3));
}

TEST(MutexStorageTestsSimple, ActiveCleanupTest) {
    MockClock::reset();
    std::vector<al::key_type> keys = {"temp"};
    
    al::MutexStorage<al::SlidingWindowAlgo, al::key_type, MockClock> storage(
        keys.begin(), keys.end(), std::make_tuple(3U, 4.0F), 2.0, 0.05, false
    );

    MockClock::advance(3s);

    std::this_thread::sleep_for(100ms);
    
    auto res = storage.Visit("temp", [](auto& algo) { return true; });
    EXPECT_TRUE(std::holds_alternative<std::false_type>(res));
}