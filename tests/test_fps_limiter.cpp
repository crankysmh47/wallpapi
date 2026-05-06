#include <doctest/doctest.h>

#include "fps_limiter.hpp"

TEST_CASE("compute_frame_sleep_ms returns 0 when disabled or behind") {
    using clock = std::chrono::steady_clock;
    const auto t0 = clock::time_point{};
    CHECK(wp::compute_frame_sleep_ms(0, t0, t0 + std::chrono::milliseconds(1)).count() == 0);
    CHECK(wp::compute_frame_sleep_ms(-1, t0, t0 + std::chrono::milliseconds(1)).count() == 0);
    CHECK(wp::compute_frame_sleep_ms(60, t0, t0 + std::chrono::milliseconds(30)).count() == 0); // >16ms
}

TEST_CASE("compute_frame_sleep_ms sleeps to reach target fps") {
    using clock = std::chrono::steady_clock;
    const auto t0 = clock::time_point{};
    // 60 fps ~ 16.67ms -> rounded to 17ms
    CHECK(wp::compute_frame_sleep_ms(60, t0, t0 + std::chrono::milliseconds(1)).count() == 16);
    // 30 fps ~ 33.33ms -> rounded to 33ms
    CHECK(wp::compute_frame_sleep_ms(30, t0, t0 + std::chrono::milliseconds(10)).count() == 23);
}

