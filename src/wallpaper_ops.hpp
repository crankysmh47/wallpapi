#pragma once
#include "wallpaper_scan.hpp"
#include <filesystem>
#include <random>
#include <shellapi.h>
#include <string>
#include <vector>

namespace wp {

inline std::vector<std::filesystem::path> list_wallpapers(
    const std::vector<std::string>& extensions
) {
    return list_files_by_extension("wallpapers", extensions);
}

inline std::string copy_into_wallpapers(const std::filesystem::path& source) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(source, ec) || !fs::is_regular_file(source, ec)) {
        return {};
    }

    fs::path dest_dir = "wallpapers";
    fs::create_directories(dest_dir, ec);

    auto dest = dest_dir / source.filename();
    if (fs::equivalent(source, dest, ec)) {
        return dest.string();
    }

    fs::copy_file(source, dest, fs::copy_options::overwrite_existing, ec);
    if (ec) return {};
    return dest.string();
}

inline bool open_wallpapers_folder() {
    const auto dir = std::filesystem::absolute("wallpapers").string();
    std::filesystem::create_directories("wallpapers");
    const auto result = reinterpret_cast<INT_PTR>(
        ShellExecuteA(nullptr, "open", dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL)
    );
    return result > 32;
}

inline std::optional<std::filesystem::path> pick_next_wallpaper(
    const std::filesystem::path& current,
    const std::vector<std::string>& extensions
) {
    const auto list = list_wallpapers(extensions);
    if (list.empty()) return std::nullopt;
    if (list.size() == 1) return list.front();

    std::filesystem::path cur = current;
    if (!cur.empty()) {
        cur = std::filesystem::weakly_canonical(cur);
    }

    size_t index = 0;
    for (size_t i = 0; i < list.size(); ++i) {
        std::error_code ec;
        if (std::filesystem::equivalent(list[i], cur, ec)) {
            index = (i + 1) % list.size();
            return list[index];
        }
    }
    return list.front();
}

inline std::optional<std::filesystem::path> pick_random_wallpaper(
    const std::vector<std::string>& extensions
) {
    const auto list = list_wallpapers(extensions);
    if (list.empty()) return std::nullopt;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, list.size() - 1);
    return list[dist(gen)];
}

} // namespace wp
