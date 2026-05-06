#include <doctest/doctest.h>

#include "mpv_options.hpp"

TEST_CASE("build_mpv_options includes mute toggle") {
    {
        wp::MpvRuntimeOptions opt;
        opt.muted = true;
        const auto o = wp::build_mpv_options(opt);
        bool found = false;
        for (const auto& kv : o) {
            if (kv.first == "mute") {
                found = true;
                CHECK(kv.second == "yes");
            }
        }
        CHECK(found);
    }
    {
        wp::MpvRuntimeOptions opt;
        opt.muted = false;
        const auto o = wp::build_mpv_options(opt);
        bool found = false;
        for (const auto& kv : o) {
            if (kv.first == "mute") {
                found = true;
                CHECK(kv.second == "no");
            }
        }
        CHECK(found);
    }
}

