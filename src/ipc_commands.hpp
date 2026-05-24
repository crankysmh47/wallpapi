#pragma once
#include <string>
#include <string_view>

namespace wp {

enum class IPCCommandType {
    Unknown = 0,
    SetVideo,
    SetImage,
    Pause,
    Resume,
    GetStatus,
    ListWallpapers,
    Stop,
    GetConfig,
    SetConfig,
    TogglePauseOnBattery,
    TogglePauseOnFullscreen,
    ToggleMuted,
    OpenWallpapers,
    AddWallpaper,
    NextWallpaper,
    RandomWallpaper,
    Help,
};

struct IPCCommand {
    IPCCommandType type = IPCCommandType::Unknown;
    std::string argument;
    std::string second_argument;
};

inline std::string_view trim(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) {
        s.remove_prefix(1);
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) {
        s.remove_suffix(1);
    }
    return s;
}

inline bool starts_with(std::string_view s, std::string_view prefix) {
    return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

inline bool parse_bool(std::string_view value, bool* out) {
    const auto v = trim(value);
    if (v == "true" || v == "1" || v == "yes" || v == "on") {
        if (out) *out = true;
        return true;
    }
    if (v == "false" || v == "0" || v == "no" || v == "off") {
        if (out) *out = false;
        return true;
    }
    return false;
}

inline IPCCommand parse_ipc_command(std::string_view raw) {
    raw = trim(raw);

    if (raw == "pause") return {IPCCommandType::Pause, {}};
    if (raw == "resume") return {IPCCommandType::Resume, {}};
    if (raw == "get-status" || raw == "status") return {IPCCommandType::GetStatus, {}};
    if (raw == "list-wallpapers" || raw == "list") return {IPCCommandType::ListWallpapers, {}};
    if (raw == "config get") return {IPCCommandType::GetConfig, {}};
    if (raw == "toggle pause_on_battery") return {IPCCommandType::TogglePauseOnBattery, {}};
    if (raw == "toggle pause_on_fullscreen") return {IPCCommandType::TogglePauseOnFullscreen, {}};
    if (raw == "toggle muted") return {IPCCommandType::ToggleMuted, {}};
    if (raw == "open-wallpapers" || raw == "open") return {IPCCommandType::OpenWallpapers, {}};
    if (raw == "next") return {IPCCommandType::NextWallpaper, {}};
    if (raw == "random") return {IPCCommandType::RandomWallpaper, {}};
    if (raw == "help") return {IPCCommandType::Help, {}};
    if (raw == "stop" || raw == "quit") return {IPCCommandType::Stop, {}};

    constexpr std::string_view kConfigSet = "config set ";
    if (starts_with(raw, kConfigSet)) {
        auto rest = trim(raw.substr(kConfigSet.size()));
        const auto space = rest.find(' ');
        if (space == std::string_view::npos) return {IPCCommandType::Unknown, {}};
        return {
            IPCCommandType::SetConfig,
            std::string(trim(rest.substr(0, space))),
            std::string(trim(rest.substr(space + 1)))
        };
    }

    constexpr std::string_view kAdd = "add ";
    if (starts_with(raw, kAdd)) {
        auto rest = trim(raw.substr(kAdd.size()));
        if (rest.empty()) return {IPCCommandType::Unknown, {}};
        return {IPCCommandType::AddWallpaper, std::string(rest)};
    }

    constexpr std::string_view kSetVideo = "set-video ";
    if (starts_with(raw, kSetVideo)) {
        auto rest = trim(raw.substr(kSetVideo.size()));
        if (rest.empty()) return {IPCCommandType::Unknown, {}};
        return {IPCCommandType::SetVideo, std::string(rest)};
    }

    constexpr std::string_view kSetImage = "set-image ";
    if (starts_with(raw, kSetImage)) {
        auto rest = trim(raw.substr(kSetImage.size()));
        if (rest.empty()) return {IPCCommandType::Unknown, {}};
        return {IPCCommandType::SetImage, std::string(rest)};
    }

    constexpr std::string_view kSet = "set ";
    if (starts_with(raw, kSet)) {
        auto rest = trim(raw.substr(kSet.size()));
        constexpr std::string_view kVideoPrefix = "video ";
        constexpr std::string_view kImagePrefix = "image ";
        if (starts_with(rest, kVideoPrefix)) {
            rest = trim(rest.substr(kVideoPrefix.size()));
        } else if (starts_with(rest, kImagePrefix)) {
            rest = trim(rest.substr(kImagePrefix.size()));
        }
        if (rest.empty()) return {IPCCommandType::Unknown, {}};
        return {IPCCommandType::SetVideo, std::string(rest)};
    }

    return {IPCCommandType::Unknown, {}};
}

} // namespace wp
