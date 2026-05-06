#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <windows.h>
#include <toml.hpp>
#include <functional>

namespace wp {

struct Config {
    std::string video_path;
    std::string shader_path;
    bool muted = true;
    int fps_limit = 60;
    bool pause_on_battery = true;
    bool pause_on_fullscreen = true;
};

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    bool load(const std::string& path);
    bool save() const;
    bool save_as(const std::string& path) const;

    // Updates current config and persists it to disk.
    // These are intended to be called by runtime commands (IPC, UI) so the
    // engine remembers the last selected wallpaper.
    bool set_current_video(const std::string& video_path);
    bool set_current_shader(const std::string& shader_path);

    void start_watching(const std::string& path, std::function<void(const Config&)> callback);
    void stop_watching();

    const Config& get_current() const { return m_current; }
    const std::string& get_path() const { return m_path; }

private:
    void watch_loop(std::string path);
    
    mutable std::mutex m_mutex;
    Config m_current;
    std::string m_path;
    std::thread m_watch_thread;
    std::atomic<bool> m_running{false};
    HANDLE m_dir_handle = INVALID_HANDLE_VALUE;
    std::function<void(const Config&)> m_callback;
};

} // namespace wp
