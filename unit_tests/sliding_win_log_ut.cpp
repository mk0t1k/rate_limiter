#include "gtest/gtest.h"
#include <cstddef>
#include <limits>
#include <thread>

#include "lib/sliding_win_log.hpp"
#include "mock_clock.hpp"

namespace al = avito_limiter;

using namespace std::chrono_literals;

class SlidingWindowTests : public ::testing::Test {
protected:
    SlidingWindowTests() : algo{3, 2.} {}
    al::SlidingWindowAlgo<MockClock> algo;
};

TEST_F(SlidingWindowTests, StandardWinLogicTest) {
    EXPECT_EQ(algo.GetNumAvail(), 3);

    EXPECT_TRUE(algo.Access());
    std::this_thread::sleep_for(600ms);
    EXPECT_TRUE(algo.Access());
    EXPECT_TRUE(algo.Access());
    std::this_thread::sleep_for(1s);
    EXPECT_FALSE(algo.Access());
    std::this_thread::sleep_for(600ms);
    EXPECT_TRUE(algo.Access());

    std::this_thread::sleep_for(2100ms);
    EXPECT_EQ(algo.GetNumAvail(), 3);
}

TEST_F(SlidingWindowTests, BadAccessTest) {
    EXPECT_TRUE(algo.Access());
    EXPECT_TRUE(algo.Access());
    EXPECT_TRUE(algo.Access());

    std::this_thread::sleep_for(500ms);
    EXPECT_FALSE(algo.Access());
    EXPECT_FALSE(algo.Access());

    std::this_thread::sleep_for(1100ms);
    EXPECT_FALSE(algo.Access());
    EXPECT_FALSE(algo.Access());

    std::this_thread::sleep_for(500ms);
    EXPECT_TRUE(algo.Access());
}


TEST(EdegCasesSlidignWindowTests, ZeroCapTest) {
    al::SlidingWindowAlgo zero_cap{0, 1.};
    EXPECT_FALSE(zero_cap.Access());
    std::this_thread::sleep_for(500ms);
    EXPECT_FALSE(zero_cap.Access());
}

TEST(EdegCasesSlidignWindowTests, ZeroWinSizeTest) {
    al::SlidingWindowAlgo zero_win{3, 0.};
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_TRUE(zero_win.Access());
    }
    EXPECT_EQ(zero_win.GetNumAvail(), 3);
}

TEST(EdegCasesSlidignWindowTests, NegWinSizeTest) {
    EXPECT_DEBUG_DEATH(al::SlidingWindowAlgo(3, -1), "");
}

TEST(EdegCasesSlidignWindowTests, InfWinSizeTest) {
    al::SlidingWindowAlgo inf_win{3, std::numeric_limits<float>::infinity()};
    EXPECT_TRUE(inf_win.Access());
    EXPECT_TRUE(inf_win.Access());
    EXPECT_TRUE(inf_win.Access());

    std::this_thread::sleep_for(2s);
    EXPECT_FALSE(inf_win.Access());

    std::this_thread::sleep_for(500ms);
    EXPECT_FALSE(inf_win.Access());
}

TEST(EdegCasesSlidignWindowTests, NANWinSizeTest) {
    EXPECT_DEBUG_DEATH(al::SlidingWindowAlgo(3, std::numeric_limits<float>::quiet_NaN()), "");
    EXPECT_DEBUG_DEATH(al::SlidingWindowAlgo(3, std::numeric_limits<float>::signaling_NaN()), "");
}




