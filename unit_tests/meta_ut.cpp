#include <gtest/gtest.h>

#include "lib/meta.hpp"

namespace am = avito_meta;

TEST(MetaTests, GcdTests) {
    static_assert(am::Gcd<1U, 0U>::value == 1U);
    static_assert(am::Gcd<0U, 0U>::value == 0U);
    static_assert(am::Gcd<0U, 1U>::value == 1U);
    static_assert(am::Gcd<5U, 10U>::value == 5U);
    static_assert(am::Gcd<204U, 170U>::value == 34U);
}

TEST(MetaTests, LcmTests) {
    static_assert(am::Lcm<1U, 0U>::value == 1U);
    static_assert(am::Lcm<0U, 0U>::value == 0U);
    static_assert(am::Lcm<0U, 1U>::value == 1U);
    static_assert(am::Lcm<5U, 10U>::value == 10U);
    static_assert(am::Lcm<204U, 170U>::value == 1020U);
}

TEST(MetaTests, ValueConditionalTests) {
    static_assert(am::ValueConditional<int, true, 1, -1>::value == 1);
    static_assert(am::ValueConditional<int, false, 1, -1>::value == -1);
    static_assert(am::ValueConditional<int, true, 1, -1>{} == 1);
    static_assert(am::ValueConditional<int, false, 1, -1>{} == -1);
}