#include "gtest/gtest.h"

#include <cstddef>
#include <gtest/gtest.h>
#include <limits>
#include <thread>

#include "lib/token_bucket.hpp"
#include "mock_clock.hpp"

namespace al = avito_limiter;

using namespace std::chrono_literals;

using AlgType = al::TokenBucketAlgo<MockClock>;

class TokenBucketTests : public ::testing::Test {
protected:

    void TearDown() override {
        MockClock::reset();
    }

    TokenBucketTests() : 
        algo(std::make_unique<AlgType>(1., 3)),
        algo_with_ttl(std::make_unique<AlgType>(10., 10, 10)) {}
    
    std::unique_ptr<al::AlgBase<AlgType, MockClock>> algo;
    std::unique_ptr<al::AlgBase<AlgType, MockClock>> algo_with_ttl;
};

TEST_F(TokenBucketTests, InitialStateTest) {
    EXPECT_EQ(algo->GetNumAvail(), 3);
}

TEST_F(TokenBucketTests, LackOfTokensTest) {
    EXPECT_TRUE(algo->Access());
    EXPECT_TRUE(algo->Access());
    EXPECT_TRUE(algo->Access());
    EXPECT_FALSE(algo->Access());
}

TEST_F(TokenBucketTests, TokensRefillTest) {
    EXPECT_TRUE(algo->Access());
    EXPECT_TRUE(algo->Access());
    EXPECT_TRUE(algo->Access());

    MockClock::advance(999ms);
    EXPECT_FALSE(algo->Access());

    MockClock::advance(1ms);
    EXPECT_TRUE(algo->Access());
    EXPECT_FALSE(algo->Access());
}

TEST_F(TokenBucketTests, ExcedingTokenLimitTest) {
    EXPECT_TRUE(algo->Access());
    EXPECT_TRUE(algo->Access());
    EXPECT_TRUE(algo->Access());
    MockClock::advance(10s);
    EXPECT_EQ(algo->GetNumAvail(), 3);
}


TEST_F(TokenBucketTests, TtlBoundsTest) {
    EXPECT_TRUE(algo_with_ttl->Access());
    
    MockClock::advance(9999ms);
    EXPECT_TRUE(algo_with_ttl->Access());

    MockClock::advance(2ms);
    EXPECT_TRUE(algo_with_ttl->Access());

    MockClock::advance(10001ms);
    EXPECT_FALSE(algo_with_ttl->Access());
}


TEST(TokenBucketEdgeCasesTests, ZeroCapacity) {
    AlgType zero_cap{1., 0};
    EXPECT_FALSE(zero_cap.Access());
}

TEST(TokenBucketEdgeCasesTests, ZeroRefill) {
    AlgType zero_refill{0., 2};
    EXPECT_TRUE(zero_refill.Access());
    EXPECT_TRUE(zero_refill.Access());
    EXPECT_FALSE(zero_refill.Access());
    MockClock::advance(10s);
    EXPECT_FALSE(zero_refill.Access());
    EXPECT_EQ(zero_refill.GetNumAvail(), 0);
}

TEST(TokenBucketEdgeCasesTests, InfinityRefillTest) {
    AlgType inf_refill{std::numeric_limits<float>::infinity(), 2};
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_TRUE(inf_refill.Access());
    }
    EXPECT_EQ(inf_refill.GetNumAvail(), 2);
}

TEST(TokenBucketEdgeCasesTests, NegativeRefillTest) {
    AlgType neg_refill{-1, 2};
    MockClock::advance(2s);
    EXPECT_FALSE(neg_refill.Access());
    MockClock::advance(2s);
    EXPECT_FALSE(neg_refill.Access());

    EXPECT_EQ(neg_refill.GetNumAvail(), 0);
}

TEST(TokenBucketEdgeCasesTests, NanRefillTest) {
    EXPECT_DEBUG_DEATH(AlgType(std::numeric_limits<float>::quiet_NaN(), 2), "");
    EXPECT_DEBUG_DEATH(AlgType(std::numeric_limits<float>::signaling_NaN(), 2), "");
}
