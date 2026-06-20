#include "gtest/gtest.h"

#include <cstddef>
#include <gtest/gtest.h>
#include <limits>
#include <thread>

#include "lib/token_bucket.hpp"

namespace al = avito_limiter;

using namespace std::chrono_literals;

class TokenBucketTest : public ::testing::Test {
protected:
    TokenBucketTest() : algo{1., 3} {}
    al::TokenBucketAlgo algo;
};

TEST_F(TokenBucketTest, InitialStateTest) {
    EXPECT_EQ(algo.GetNumAvail(), 3);
}

TEST_F(TokenBucketTest, LackOfTokensTest) {
    EXPECT_TRUE(algo.Access());
    EXPECT_TRUE(algo.Access());
    EXPECT_TRUE(algo.Access());
    EXPECT_FALSE(algo.Access());
}

TEST_F(TokenBucketTest, TokensRefillTest) {
    EXPECT_TRUE(algo.Access());
    EXPECT_TRUE(algo.Access());
    EXPECT_TRUE(algo.Access());

    std::this_thread::sleep_for(1.1s);

    EXPECT_TRUE(algo.Access());
    EXPECT_FALSE(algo.Access());
}

TEST_F(TokenBucketTest, TokensCountAfterQueriesTest) {
    al::TokenBucketAlgo algo {0.8, 2};
    algo.Access();
    algo.Access();
    EXPECT_EQ(algo.GetNumAvail(), 0);
    std::this_thread::sleep_for(500ms);
    EXPECT_EQ(algo.GetNumAvail(), 0);
    std::this_thread::sleep_for(1500ms);
    EXPECT_EQ(algo.GetNumAvail(), 1);
}

TEST_F(TokenBucketTest, ExcedingTokenLimitTest) {
    EXPECT_TRUE(algo.Access());
    EXPECT_TRUE(algo.Access());
    EXPECT_TRUE(algo.Access());
    std::this_thread::sleep_for(6s);
    EXPECT_EQ(algo.GetNumAvail(), 3);
}

TEST_F(TokenBucketTest, ZeroCapacity) {
    al::TokenBucketAlgo zero_cap{1., 0};
    EXPECT_FALSE(zero_cap.Access());
}

TEST_F(TokenBucketTest, ZeroRefill) {
    al::TokenBucketAlgo zero_refill{0., 2};
    EXPECT_TRUE(zero_refill.Access());
    EXPECT_TRUE(zero_refill.Access());
    EXPECT_FALSE(zero_refill.Access());
    std::this_thread::sleep_for(1s);
    EXPECT_FALSE(zero_refill.Access());
}

TEST_F(TokenBucketTest, InfinityRefillTest) {
    al::TokenBucketAlgo inf_refill{std::numeric_limits<float>::infinity(), 2};
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_TRUE(inf_refill.Access());
    }
    EXPECT_EQ(inf_refill.GetNumAvail(), 2);
}

TEST_F(TokenBucketTest, NegativeRefillTest) {
    al::TokenBucketAlgo neg_refill{-1, 2};
    std::this_thread::sleep_for(2s);
    EXPECT_FALSE(neg_refill.Access());
    std::this_thread::sleep_for(1s);
    EXPECT_FALSE(neg_refill.Access());
}

TEST_F(TokenBucketTest, NanRefillTest) {
    EXPECT_DEBUG_DEATH(al::TokenBucketAlgo(std::numeric_limits<float>::quiet_NaN(), 2), "");
    EXPECT_DEBUG_DEATH(al::TokenBucketAlgo(std::numeric_limits<float>::signaling_NaN(), 2), "");
}
