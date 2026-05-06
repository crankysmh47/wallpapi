#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <windows.h>
#include <functional>

namespace wp {

class IPCServer {
public:
    explicit IPCServer(std::string pipe_name = "\\\\.\\pipe\\wp_engine_pipe");
    ~IPCServer();

    // Callback returns a response string (sent back to the client).
    // Convention:
    // - "OK ..." for success
    // - "ERR ..." for failures
    void start(std::function<std::string(const std::string&)> callback);
    void stop();

private:
    void server_loop();
    std::string process_command(const std::string& cmd);
    void poke_stop() const;

    std::string m_pipe_name;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::function<std::string(const std::string&)> m_callback;
};

} // namespace wp
