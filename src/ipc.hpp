#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <windows.h>
#include <functional>

namespace wp {

class IPCServer {
public:
    IPCServer();
    ~IPCServer();

    void start(std::function<void(const std::string&)> callback);
    void stop();

private:
    void server_loop();
    void process_command(const std::string& cmd);

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::function<void(const std::string&)> m_callback;
};

} // namespace wp
