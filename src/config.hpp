#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <windows.h>
#include <toml.hpp>
#include <functional>

namespace wp {

struct Config {
    std::string video_path;
    std::string shader_path;
    bool muted = true;
};

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    bool load(const std::string& path);
    void start_watching(const std::string& path, std::function<void(const Config&)> callback);
    void stop_watching();

    const Config& get_current() const { return m_current; }

private:
    void watch_loop(std::string path);
    
    Config m_current;
    std::thread m_watch_thread;
    std::atomic<bool> m_running{false};
    HANDLE m_dir_handle = INVALID_HANDLE_VALUE;
    std::function<void(const Config&)> m_callback;
};

} // namespace wp
