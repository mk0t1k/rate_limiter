#include "gtest/gtest.h"
#include <cstddef>
#include <limits>
#include <memory>
#include <thread>

#include "lib/alg_base.hpp"
#include "lib/sliding_win_log.hpp"
#include "mock_clock.hpp"

namespace al = avito_limiter;

using namespace std::chrono_literals;

using AlgType = al::SlidingWindowAlgo<MockClock>;

class SlidingWindowTests : public ::testing::Test {
protected:

    void TearDown() override {
        MockClock::reset();
    }
    
    SlidingWindowTests() : 
        algo(std::make_unique<AlgType>(3, 2.)),
        algo_with_ttl(std::make_unique<AlgType>(10, 2., 10)) {}

    std::unique_ptr<al::AlgBase<AlgType, MockClock>> algo;
    std::unique_ptr<al::AlgBase<AlgType, MockClock>> algo_with_ttl;
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
    AlgType zero_cap{0, 1.};
    EXPECT_FALSE(zero_cap.Access());
    MockClock::advance(1000s);
    EXPECT_FALSE(zero_cap.Access());
}

TEST(EdgeCasesSlidingWindowTests, ZeroWinSizeTest) {
    AlgType zero_win{3, 0.};
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_TRUE(zero_win.Access());
    }
    EXPECT_EQ(zero_win.GetNumAvail(), 3);
}

TEST(EdgeCasesSlidingWindowTests, InfWinSizeTest) {
    AlgType inf_win{3, std::numeric_limits<float>::infinity()};
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
    EXPECT_DEBUG_DEATH(AlgType(3, -1), "");
}

TEST(EdgeCasesSlidingWindowTests, NANWinSizeTest) {
    EXPECT_DEBUG_DEATH(AlgType(3, std::numeric_limits<float>::quiet_NaN()), "");
    EXPECT_DEBUG_DEATH(AlgType(3, std::numeric_limits<float>::signaling_NaN()), "");
}




