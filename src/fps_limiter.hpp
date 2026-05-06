#pragma once
#include <chrono>
#include <cstdint>
#include <cmath>

namespace wp {

// Returns how long we should sleep to respect a target FPS.
// - If fps_limit <= 0, returns 0.
// - If we're behind schedule, returns 0.
inline std::chrono::milliseconds compute_frame_sleep_ms(
    int fps_limit,
    std::chrono::steady_clock::time_point frame_start,
    std::chrono::steady_clock::time_point frame_end
) {
    if (fps_limit <= 0) return std::chrono::milliseconds(0);
    const auto frame_time = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start);
    const auto target = std::chrono::milliseconds((int)std::llround(1000.0 / (double)fps_limit));
    if (frame_time >= target) return std::chrono::milliseconds(0);
    return target - frame_time;
}

} // namespace wp

