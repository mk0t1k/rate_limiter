#pragma once

#include <chrono>


struct MockClock {
    using duration = std::chrono::nanoseconds;
    using rep = duration::rep;
    using period = duration::period;
    using time_point = std::chrono::time_point<MockClock, duration>;
    static constexpr bool is_steady = true;

    static time_point now() noexcept {
        return current_time;
    }

    static void advance(duration d) noexcept {
        current_time += d;
    }

    static void reset() noexcept {
        current_time = time_point{duration{0}};
    }

    inline static time_point current_time{duration{0}};
};
