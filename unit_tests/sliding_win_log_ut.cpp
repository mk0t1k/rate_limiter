#include "gtest/gtest.h"
#include <cstddef>
#include <limits>
#include <memory>
#include <thread>

#include "lib/algorithms/alg_base.hpp"
#include "lib/algorithms/sliding_win_log.hpp"
#include "mock_clock.hpp"

namespace al = avito_limiter;

using namespace std::chrono_literals;

using AlgType = al::StoredData<al::SlidingWindowAlgo, MockClock>;

class SlidingWindowTests : public ::testing::Test {
protected:

    void TearDown() override {
        MockClock::reset();
    }
    
    SlidingWindowTests() : 
        algo(std::make_unique<AlgType>(std::nullopt, 3UZ, 2.0F)),
        algo_with_ttl(std::make_unique<AlgType>(10.0, 10UZ, 2.0F)) {}

    std::unique_ptr<AlgType> algo;
    std::unique_ptr<AlgType> algo_with_ttl;
};


TEST_F(SlidingWindowTests, WindowBoundsTest) {
    EXPECT_EQ(algo->GetNumAvail(), 3);

    EXPECT_TRUE(algo->Access());
    EXPECT_TRUE(algo->Access());
    EXPECT_TRUE(algo->Access());

    MockClock::advance(2000ms);
    EXPECT_FALSE(algo->Access());

    MockClock::advance(1ms);
    EXPECT_TRUE(algo->Access());

    MockClock::advance(2001ms);
    EXPECT_EQ(algo->GetNumAvail(), 3);
}

TEST_F(SlidingWindowTests, BurstTest) {
    EXPECT_TRUE(algo->Access());
    EXPECT_TRUE(algo->Access());
    EXPECT_TRUE(algo->Access());

    MockClock::advance(500ms);
    EXPECT_FALSE(algo->Access());

    MockClock::advance(1000ms);
    EXPECT_FALSE(algo->Access());

    MockClock::advance(500ms);
    EXPECT_FALSE(algo->Access());
}


TEST_F(SlidingWindowTests, TtlBoundsTest) {
    EXPECT_TRUE(algo_with_ttl->Access());
    
    MockClock::advance(9999ms);
    EXPECT_TRUE(algo_with_ttl->Access());

    MockClock::advance(2ms);
    EXPECT_TRUE(algo_with_ttl->Access());

    MockClock::advance(10001ms);
    EXPECT_FALSE(algo_with_ttl->Access());
}


TEST(EdgeCasesSlidingWindowTests, ZeroCapTest) {
    AlgType zero_cap{std::nullopt, 0UZ, 1.0F};
    EXPECT_FALSE(zero_cap.Access());
    MockClock::advance(1000s);
    EXPECT_FALSE(zero_cap.Access());
}

TEST(EdgeCasesSlidingWindowTests, ZeroWinSizeTest) {
    AlgType zero_win{std::nullopt, 3UZ, 0.0F};
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_TRUE(zero_win.Access());
    }
    EXPECT_EQ(zero_win.GetNumAvail(), 3);
}

TEST(EdgeCasesSlidingWindowTests, InfWinSizeTest) {
    AlgType inf_win{std::nullopt, 3UZ, std::numeric_limits<float>::infinity()};
    EXPECT_TRUE(inf_win.Access());
    EXPECT_TRUE(inf_win.Access());
    EXPECT_TRUE(inf_win.Access());

    MockClock::advance(500s);
    EXPECT_FALSE(inf_win.Access());

    MockClock::advance(500s);
    EXPECT_FALSE(inf_win.Access());

    EXPECT_EQ(inf_win.GetNumAvail(), 0);
}

TEST(EdgeCasesSlidingWindowTests, NegWinSizeTest) {
    EXPECT_DEBUG_DEATH(AlgType(std::nullopt, 3UZ, -1.0F), "");
}

TEST(EdgeCasesSlidingWindowTests, NANWinSizeTest) {
    EXPECT_DEBUG_DEATH(AlgType(std::nullopt, 3UZ, std::numeric_limits<float>::quiet_NaN()), "");
    EXPECT_DEBUG_DEATH(AlgType(std::nullopt, 3UZ, std::numeric_limits<float>::signaling_NaN()), "");
}
