#include <cstddef>
#include <gtest/gtest.h>
#include <lib/algorithms/leaky_bucket.hpp>
#include <chrono>
#include <future>
#include <optional>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
namespace al = avito_limiter;

template<typename T>
bool IsFutureReady(const std::future<T>& fut) {
    return fut.valid() && fut.wait_for(0ms) == std::future_status::ready;
}

TEST(LeakingBucketTests, CapacityLimitsAndOverflow) {

    al::LeakyBucketShaper shaper(3, 1, 100.0);

    EXPECT_EQ(shaper.GetNumAvail(), 3);

    auto f1 = shaper.AddRequest("user1");
    ASSERT_TRUE(f1.has_value());
    EXPECT_EQ(shaper.GetNumAvail(), 2);

    auto f2 = shaper.AddRequest("user2");
    ASSERT_TRUE(f2.has_value());
    EXPECT_EQ(shaper.GetNumAvail(), 1);

    auto f3 = shaper.AddRequest("user3");
    ASSERT_TRUE(f3.has_value());
    EXPECT_EQ(shaper.GetNumAvail(), 0);

    auto f4 = shaper.AddRequest("user4");
    EXPECT_FALSE(f4.has_value());
    EXPECT_EQ(shaper.GetNumAvail(), 0);
}

TEST(LeakingBucketTests, DrainingWithForceTrigger) {
    al::LeakyBucketShaper shaper(5, 2, 100.0);

    auto opt_f1 = shaper.AddRequest("req1");
    auto opt_f2 = shaper.AddRequest("req2");
    auto opt_f3 = shaper.AddRequest("req3");

    ASSERT_TRUE(opt_f1.has_value());
    ASSERT_TRUE(opt_f2.has_value());
    ASSERT_TRUE(opt_f3.has_value());

    auto f1 = std::move(*opt_f1);
    auto f2 = std::move(*opt_f2);
    auto f3 = std::move(*opt_f3);

    EXPECT_FALSE(IsFutureReady(f1));
    EXPECT_FALSE(IsFutureReady(f2));
    EXPECT_FALSE(IsFutureReady(f3));

    shaper.ForceTrigger();

    std::this_thread::sleep_for(5ms);

    EXPECT_TRUE(IsFutureReady(f1));
    EXPECT_TRUE(IsFutureReady(f2));
    EXPECT_FALSE(IsFutureReady(f3));

    EXPECT_TRUE(f1.get());
    EXPECT_TRUE(f2.get());

    shaper.ForceTrigger();
    std::this_thread::sleep_for(5ms);

    EXPECT_TRUE(IsFutureReady(f3));
    EXPECT_TRUE(f3.get());
}

TEST(LeakingBucketTests, AutoPeriodDraining) {
    al::LeakyBucketShaper shaper(3, 1, 0.05);

    auto opt_f1 = shaper.AddRequest("req1");
    ASSERT_TRUE(opt_f1.has_value());
    auto f1 = std::move(*opt_f1);

    EXPECT_FALSE(IsFutureReady(f1));

    std::this_thread::sleep_for(100ms);

    EXPECT_TRUE(IsFutureReady(f1));
    EXPECT_TRUE(f1.get());
}

TEST(LeakingBucketTests, StressTest) {
    for (size_t attempt = 0; attempt < 1000; ++attempt) {
        al::LeakyBucketShaper shaper(3, 1, 0.001);

        std::vector<std::jthread> ths;

        size_t mx_ths = std::max(1u, std::thread::hardware_concurrency());

        for (size_t i = 0; i < mx_ths; ++i) {
            ths.emplace_back([&](){
                for (int j = 0; j < 10000; ++j) {
                    shaper.AddRequest(std::to_string(j));
                }

                EXPECT_TRUE(shaper.GetNumAvail() <= 3);
            });
        }
    }
}

TEST(EdgeCasesLeakingBucketTests, ZeroCap) {
    al::LeakyBucketShaper shaper(0, 1, 100);
    auto opt_f1 = shaper.AddRequest("req1");
    ASSERT_FALSE(opt_f1.has_value());
}

TEST(EdgeCasesLeakingBucketTests, ZeroDrain) {
    al::LeakyBucketShaper shaper(5, 0, 100);
    auto opt_f = shaper.AddRequest("req1");
    ASSERT_TRUE(opt_f.has_value());
    auto f = std::move(*opt_f);

    EXPECT_FALSE(IsFutureReady(f));

    shaper.ForceTrigger();
    std::this_thread::sleep_for(5ms);

    EXPECT_FALSE(IsFutureReady(f));
}