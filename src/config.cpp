#include "config.hpp"
#include "logger.hpp"
#include <filesystem>

namespace wp {

ConfigManager::ConfigManager() {}
ConfigManager::~ConfigManager() { stop_watching(); }

bool ConfigManager::load(const std::string& path) {
    try {
        auto data = toml::parse(path);
        m_current.video_path = toml::find_or<std::string>(data, "video", "");
        m_current.shader_path = toml::find_or<std::string>(data, "shader", "");
        m_current.muted = toml::find_or<bool>(data, "muted", true);
        
        WP_INFO("Config loaded: video={}, shader={}, muted={}", 
            m_current.video_path, m_current.shader_path, m_current.muted);
        return true;
    } catch (const std::exception& e) {
        WP_ERROR("Failed to parse config: {}", e.what());
        return false;
    }
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
    std::filesystem::path p(path);
    auto dir = p.parent_path();
    auto filename = p.filename();

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
        WP_ERROR("Failed to open directory for watching: {}", dir.string());
        return;
    }

    char buffer[4096];
    DWORD bytes_returned;

    while (m_running) {
        if (ReadDirectoryChangesW(
            m_dir_handle, buffer, sizeof(buffer), FALSE,
            FILE_NOTIFY_CHANGE_LAST_WRITE, &bytes_returned, nullptr, nullptr
        )) {
            // Debounce or wait a bit for file to be fully written
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
