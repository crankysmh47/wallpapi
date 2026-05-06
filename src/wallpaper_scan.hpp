#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace wp {

// Finds the first file in `dir` whose extension matches `extensions` (case-insensitive),
// in lexicographic order. Returns empty optional if none found.
inline std::optional<std::filesystem::path> find_first_by_extension(
    const std::filesystem::path& dir,
    const std::vector<std::string>& extensions
) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return std::nullopt;

    auto lower = [](std::string s) {
        for (char& c : s) c = (char)tolower((unsigned char)c);
        return s;
    };

    std::vector<std::string> exts;
    exts.reserve(extensions.size());
    for (auto e : extensions) {
        if (!e.empty() && e.front() != '.') e = "." + e;
        exts.push_back(lower(e));
    }

    std::vector<fs::path> files;
    for (auto it = fs::directory_iterator(dir, ec); !ec && it != fs::directory_iterator(); it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        files.push_back(it->path());
    }
    std::sort(files.begin(), files.end());

    for (const auto& p : files) {
        auto ext = lower(p.extension().string());
        for (const auto& want : exts) {
            if (ext == want) return p;
        }
    }
    return std::nullopt;
}

inline std::vector<std::filesystem::path> list_files_by_extension(
    const std::filesystem::path& dir,
    const std::vector<std::string>& extensions
) {
    namespace fs = std::filesystem;
    std::vector<fs::path> out;
    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return out;

    auto lower = [](std::string s) {
        for (char& c : s) c = (char)tolower((unsigned char)c);
        return s;
    };

    std::vector<std::string> exts;
    exts.reserve(extensions.size());
    for (auto e : extensions) {
        if (!e.empty() && e.front() != '.') e = "." + e;
        exts.push_back(lower(e));
    }

    for (auto it = fs::directory_iterator(dir, ec); !ec && it != fs::directory_iterator(); it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        auto ext = lower(it->path().extension().string());
        for (const auto& want : exts) {
            if (ext == want) {
                out.push_back(it->path());
                break;
            }
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

} // namespace wp

