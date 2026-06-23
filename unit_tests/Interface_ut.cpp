
#include <gtest/gtest.h>

#include <lib/algorithms/limiters.hpp>

namespace al = avito_limiter;

TEST(IntegrationTests, MutexWinLimiterTest) {
    al::Factory factory;
    std::unique_ptr<al::IRateLimiter> limiter(
        factory.Allocate<al::MutexWinLimiter>()
    );

    al::key_type key = "user_1";

    EXPECT_EQ(limiter->GetNumAvail(key), 0); 
    EXPECT_FALSE(limiter->Access(key)); 
}