
#include "gtest/gtest.h"
#include <gtest/gtest.h>
#include <lib/ttl.hpp>
#include <limits>
#include <optional>

#include "mock_clock.hpp"

using namespace std::chrono_literals;

namespace al = avito_limiter;

class TtlTests : public ::testing::Test {
protected:

    void TearDown() override {
        MockClock::reset();
    }

    TtlTests() : 
        default_ttl(),
        ttl(5) {}
    
    al::TtlValue<MockClock> default_ttl;
    al::TtlValue<MockClock> ttl;
};

TEST_F(TtlTests, defaultTest) {
    EXPECT_TRUE(static_cast<bool>(default_ttl));
    EXPECT_EQ(default_ttl.GetTtl(), std::nullopt);

    MockClock::advance(10000s);
    EXPECT_TRUE(static_cast<bool>(default_ttl));
    EXPECT_EQ(default_ttl.GetTtl(), std::nullopt);
}

TEST_F(TtlTests, FiniteTtlTest) {
    EXPECT_TRUE(static_cast<bool>(ttl));
    EXPECT_DOUBLE_EQ(*ttl.GetTtl(), 5);

    MockClock::advance(3s);
    EXPECT_TRUE(static_cast<bool>(ttl));
    EXPECT_DOUBLE_EQ(*ttl.GetTtl(), 2);

    MockClock::advance(1999ms);
    EXPECT_TRUE(static_cast<bool>(ttl));
    EXPECT_DOUBLE_EQ(*ttl.GetTtl(), 0.001);

    MockClock::advance(2ms);
    EXPECT_FALSE(static_cast<bool>(ttl));
}

TEST_F(TtlTests, SetTest) {

    ttl.Set(std::nullopt);

    EXPECT_TRUE(static_cast<bool>(ttl));
    EXPECT_EQ(ttl.GetTtl(), std::nullopt);

    MockClock::advance(10000s);
    EXPECT_TRUE(static_cast<bool>(ttl));
    EXPECT_EQ(ttl.GetTtl(), std::nullopt);

    ttl.Set(10);
    MockClock::advance(3s);
    EXPECT_TRUE(static_cast<bool>(ttl));
    EXPECT_DOUBLE_EQ(*ttl.GetTtl(), 7);

    ttl.Set(5);
    EXPECT_TRUE(static_cast<bool>(ttl));
    EXPECT_DOUBLE_EQ(*ttl.GetTtl(), 5);
}

TEST_F(TtlTests, CmpTest) {
    al::TtlValue<MockClock> inf1 {};
    al::TtlValue<MockClock> inf2 {};
    al::TtlValue<MockClock> finite1 {5};
    al::TtlValue<MockClock> finite2 {10};
    al::TtlValue<MockClock> finite3 {10};

    EXPECT_TRUE(inf1 == inf2);
    EXPECT_TRUE(inf1 == inf1);
    EXPECT_TRUE(inf1 > finite1 && inf1 > finite2);
    EXPECT_TRUE(finite2 > finite1);
    EXPECT_TRUE(finite2 == finite3);

    MockClock::advance(4s);
    al::TtlValue<MockClock> new_finite {2};

    EXPECT_TRUE(new_finite > finite1);
}

TEST(EdgeCasesTests, ZeroTtl) {
    MockClock::reset();

    al::TtlValue<MockClock> zero_ttl {0};
    
    EXPECT_TRUE(static_cast<bool>(zero_ttl));
    EXPECT_DOUBLE_EQ(*zero_ttl.GetTtl(), 0);

    MockClock::advance(1ns);
    EXPECT_FALSE(static_cast<bool>(zero_ttl));
}

TEST(EdgeCasesTests, InfTtl) {
    MockClock::reset();

    al::TtlValue<MockClock> inf_ttl {std::numeric_limits<double>::infinity()};
    EXPECT_TRUE(static_cast<bool>(inf_ttl));

    MockClock::advance(24h * 365 * 10);
    EXPECT_TRUE(static_cast<bool>(inf_ttl));
}

TEST(EdgeCasesTests, NegTtl) {
    MockClock::reset();

    al::TtlValue<MockClock> neg_ttl {-1};
    EXPECT_FALSE(static_cast<bool>(neg_ttl));

    MockClock::advance(24h * 365 * 10);
    EXPECT_FALSE(static_cast<bool>(neg_ttl));
}

TEST(EdgeCasesTests, NanTtl) {
    MockClock::reset();
    
    EXPECT_DEBUG_DEATH(al::TtlValue<MockClock>(std::optional(std::numeric_limits<double>::quiet_NaN())), "");
    EXPECT_DEBUG_DEATH(al::TtlValue<MockClock>(std::optional(std::numeric_limits<double>::signaling_NaN())), "");
}