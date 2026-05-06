#include <doctest/doctest.h>

#include "fullscreen_detect.hpp"

TEST_CASE("is_fullscreen_on_monitor detects coverage") {
    RECT mon{0, 0, 1920, 1080};

    RECT exact{0, 0, 1920, 1080};
    CHECK(wp::is_fullscreen_on_monitor(exact, mon));

    RECT larger{-10, -10, 1930, 1090};
    CHECK(wp::is_fullscreen_on_monitor(larger, mon));

    RECT smaller{0, 0, 1919, 1079};
    CHECK(!wp::is_fullscreen_on_monitor(smaller, mon));

    RECT shifted{10, 0, 1930, 1080};
    CHECK(!wp::is_fullscreen_on_monitor(shifted, mon));
}

