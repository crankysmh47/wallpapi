#include "ipc.hpp"
#include "logger.hpp"
#include <sstream>
#include <iomanip>

namespace wp {

IPCServer::IPCServer() {}
IPCServer::~IPCServer() { stop(); }

void IPCServer::start(std::function<void(const std::string&)> callback) {
    m_callback = callback;
    m_running = true;
    m_thread = std::thread(&IPCServer::server_loop, this);
}

void IPCServer::stop() {
    m_running = false;
    // We might need to send a dummy message to the pipe to unblock ConnectNamedPipe
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void IPCServer::server_loop() {
    const char* pipe_name = "\\\\.\\pipe\\wp_engine_pipe";

    while (m_running) {
        HANDLE pipe = CreateNamedPipeA(
            pipe_name,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, 512, 512, 0, nullptr
        );

        if (pipe == INVALID_HANDLE_VALUE) {
            WP_ERROR("Failed to create named pipe");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        if (ConnectNamedPipe(pipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) {
            char buffer[512];
            DWORD bytes_read;
            if (ReadFile(pipe, buffer, sizeof(buffer) - 1, &bytes_read, nullptr)) {
                buffer[bytes_read] = '\0';
                process_command(std::string(buffer));
            }
        }

        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }
}

void IPCServer::process_command(const std::string& cmd) {
    WP_INFO("Received IPC command: {}", cmd);
    if (m_callback) {
        m_callback(cmd);
    }
}

} // namespace wp
