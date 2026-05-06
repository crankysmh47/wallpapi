#pragma once
#include <string>
#include <utility>
#include <vector>

namespace wp {

struct MpvRuntimeOptions {
    bool muted = true;
};

inline std::vector<std::pair<std::string, std::string>> build_mpv_options(const MpvRuntimeOptions& opt) {
    std::vector<std::pair<std::string, std::string>> o;
    // Keep mpv fully managed by our app (no user config or input).
    o.emplace_back("config", "no");
    o.emplace_back("idle", "yes");
    o.emplace_back("vo", "gpu-next");
    o.emplace_back("hwdec", "auto");
    o.emplace_back("loop-file", "inf");
    o.emplace_back("input-default-bindings", "no");
    o.emplace_back("input-vo-keyboard", "no");
    o.emplace_back("mute", opt.muted ? "yes" : "no");
    return o;
}

} // namespace wp

