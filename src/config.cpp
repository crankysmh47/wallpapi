#include "config.hpp"
#include "logger.hpp"
#include <filesystem>
#include <fstream>

namespace wp {

static std::string resolve_relative_to_config_dir(const std::string& config_path, const std::string& value) {
    if (value.empty()) return value;

    std::filesystem::path p(value);
    if (p.is_absolute()) return value;

    std::filesystem::path cfg(config_path);
    auto dir = std::filesystem::absolute(cfg).parent_path();
    if (dir.empty()) dir = ".";

    std::error_code ec;
    auto combined = std::filesystem::weakly_canonical(dir / p, ec);
    if (ec) {
        combined = dir / p;
    }
    return combined.string();
}

static bool write_config_file(const std::string& path, const Config& cfg) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;

    auto quote = [](const std::string& s) -> std::string {
        std::string r;
        r.reserve(s.size() + 2);
        r.push_back('"');
        for (char c : s) {
            switch (c) {
                case '\\': r += "\\\\"; break;
                case '"': r += "\\\""; break;
                case '\n': r += "\\n"; break;
                case '\r': r += "\\r"; break;
                case '\t': r += "\\t"; break;
                default: r.push_back(c); break;
            }
        }
        r.push_back('"');
        return r;
    };

    out << "video = " << quote(cfg.video_path) << "\n";
    out << "muted = " << (cfg.muted ? "true" : "false") << "\n";
    out << "pause_on_battery = " << (cfg.pause_on_battery ? "true" : "false") << "\n";
    out << "pause_on_fullscreen = " << (cfg.pause_on_fullscreen ? "true" : "false") << "\n";
    return true;
}

ConfigManager::ConfigManager() {}
ConfigManager::~ConfigManager() { stop_watching(); }

bool ConfigManager::load(const std::string& path) {
    try {
        auto data = toml::parse(path);
        Config next;
        next.video_path = toml::find_or<std::string>(data, "video", "");
        next.muted = toml::find_or<bool>(data, "muted", true);
        next.pause_on_battery = toml::find_or<bool>(data, "pause_on_battery", true);
        next.pause_on_fullscreen = toml::find_or<bool>(data, "pause_on_fullscreen", true);

        next.video_path = resolve_relative_to_config_dir(path, next.video_path);
        if (!next.video_path.empty() && !std::filesystem::exists(next.video_path)) {
            WP_WARN("Config video path does not exist, ignoring: {}", next.video_path);
            next.video_path.clear();
        }

        {
            std::scoped_lock lk(m_mutex);
            m_current = std::move(next);
            m_path = path;
        }

        WP_INFO("Config loaded: video={}, muted={}, pause_on_battery={}, pause_on_fullscreen={}",
            m_current.video_path,
            m_current.muted,
            m_current.pause_on_battery,
            m_current.pause_on_fullscreen);
        return true;
    } catch (const std::exception& e) {
        WP_ERROR("Failed to parse config: {}", e.what());
        return false;
    }
}

bool ConfigManager::save() const {
    std::scoped_lock lk(m_mutex);
    if (m_path.empty()) return false;
    return write_config_file(m_path, m_current);
}

bool ConfigManager::set_current_video(const std::string& video_path) {
    std::scoped_lock lk(m_mutex);
    if (m_path.empty()) return false;
    const auto resolved = resolve_relative_to_config_dir(m_path, video_path);
    if (!std::filesystem::exists(resolved)) {
        WP_ERROR("Refusing to save missing video path: {}", resolved);
        return false;
    }
    m_current.video_path = resolved;
    return write_config_file(m_path, m_current);
}

bool ConfigManager::set_muted(bool muted) {
    std::scoped_lock lk(m_mutex);
    if (m_path.empty()) return false;
    m_current.muted = muted;
    return write_config_file(m_path, m_current);
}

bool ConfigManager::set_pause_on_battery(bool value) {
    std::scoped_lock lk(m_mutex);
    if (m_path.empty()) return false;
    m_current.pause_on_battery = value;
    return write_config_file(m_path, m_current);
}

bool ConfigManager::set_pause_on_fullscreen(bool value) {
    std::scoped_lock lk(m_mutex);
    if (m_path.empty()) return false;
    m_current.pause_on_fullscreen = value;
    return write_config_file(m_path, m_current);
}

void ConfigManager::start_watching(const std::string& path, std::function<void(const Config&)> callback) {
    m_callback = callback;
    m_running = true;
    m_watch_thread = std::thread(&ConfigManager::watch_loop, this, path);
}

void ConfigManager::stop_watching() {
    m_running = false;
    if (m_dir_handle != INVALID_HANDLE_VALUE) {
        CancelIoEx(m_dir_handle, nullptr);
    }
    if (m_watch_thread.joinable()) {
        m_watch_thread.join();
    }
}

void ConfigManager::watch_loop(std::string path) {
    std::filesystem::path p = std::filesystem::absolute(path);
    auto dir = p.parent_path();
    if (dir.empty()) dir = ".";

    m_dir_handle = CreateFileA(
        dir.string().c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr
    );

    if (m_dir_handle == INVALID_HANDLE_VALUE) {
        WP_ERROR("Failed to open directory for watching: '{}'", dir.string());
        return;
    }

    char buffer[4096];
    DWORD bytes_returned;

    while (m_running) {
        if (ReadDirectoryChangesW(
            m_dir_handle, buffer, sizeof(buffer), FALSE,
            FILE_NOTIFY_CHANGE_LAST_WRITE, &bytes_returned, nullptr, nullptr
        )) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (load(path) && m_callback) {
                m_callback(m_current);
            }
        }
    }

    CloseHandle(m_dir_handle);
    m_dir_handle = INVALID_HANDLE_VALUE;
}

} // namespace wp
