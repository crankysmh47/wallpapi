#pragma once
#include <string>
#include <string_view>

namespace wp {

enum class IPCCommandType {
    Unknown = 0,
    SetVideo,
    SetImage,
    SetShader,
    Pause,
    Resume,
    GetStatus,
    ListWallpapers,
    Stop,
};

struct IPCCommand {
    IPCCommandType type = IPCCommandType::Unknown;
    std::string argument; // path or empty
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

// Parses the raw command string received over IPC.
// Supported:
// - "set-video <path>"
// - "set-image <path>" (alias of set-video; mpv supports images)
// - "set-shader <path>"
// - "pause"
// - "resume"
// - "get-status"
// - "list-wallpapers"
// - "stop"
inline IPCCommand parse_ipc_command(std::string_view raw) {
    raw = trim(raw);
    if (raw == "pause") {
        return {IPCCommandType::Pause, {}};
    }
    if (raw == "resume") {
        return {IPCCommandType::Resume, {}};
    }
    if (raw == "get-status" || raw == "status") {
        return {IPCCommandType::GetStatus, {}};
    }
    if (raw == "list-wallpapers" || raw == "list") {
        return {IPCCommandType::ListWallpapers, {}};
    }
    if (raw == "stop" || raw == "quit") {
        return {IPCCommandType::Stop, {}};
    }

    constexpr std::string_view kSetVideo = "set-video";
    constexpr std::string_view kSetImage = "set-image";
    constexpr std::string_view kSetShader = "set-shader";

    if (starts_with(raw, kSetVideo)) {
        auto rest = trim(raw.substr(kSetVideo.size()));
        if (!rest.empty() && rest.front() == ' ') rest = trim(rest);
        if (rest.empty()) return {IPCCommandType::Unknown, {}};
        return {IPCCommandType::SetVideo, std::string(rest)};
    }

    if (starts_with(raw, kSetImage)) {
        auto rest = trim(raw.substr(kSetImage.size()));
        if (!rest.empty() && rest.front() == ' ') rest = trim(rest);
        if (rest.empty()) return {IPCCommandType::Unknown, {}};
        return {IPCCommandType::SetImage, std::string(rest)};
    }

    if (starts_with(raw, kSetShader)) {
        auto rest = trim(raw.substr(kSetShader.size()));
        if (!rest.empty() && rest.front() == ' ') rest = trim(rest);
        if (rest.empty()) return {IPCCommandType::Unknown, {}};
        return {IPCCommandType::SetShader, std::string(rest)};
    }

    return {IPCCommandType::Unknown, {}};
}

} // namespace wp

