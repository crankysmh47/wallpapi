#include "ipc.hpp"
#include "logger.hpp"
#include <sstream>
#include <iomanip>
#include <vector>

namespace wp {

IPCServer::IPCServer(std::string pipe_name) : m_pipe_name(std::move(pipe_name)) {}
IPCServer::~IPCServer() { stop(); }

void IPCServer::start(std::function<std::string(const std::string&)> callback) {
    m_callback = callback;
    m_running = true;
    m_thread = std::thread(&IPCServer::server_loop, this);
}

void IPCServer::stop() {
    m_running = false;
    poke_stop();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void IPCServer::poke_stop() const {
    // Connect briefly to unblock ConnectNamedPipe() so the thread can exit.
    HANDLE h = CreateFileA(
        m_pipe_name.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr,
        OPEN_EXISTING,
        0, nullptr
    );
    if (h != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        const char* msg = "noop";
        WriteFile(h, msg, (DWORD)strlen(msg), &written, nullptr);
        CloseHandle(h);
    }
}

void IPCServer::server_loop() {
    while (m_running) {
        HANDLE pipe = CreateNamedPipeA(
            m_pipe_name.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES, 4096, 4096, 0, nullptr
        );

        if (pipe == INVALID_HANDLE_VALUE) {
            WP_ERROR("Failed to create named pipe");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        if (ConnectNamedPipe(pipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) {
            char buffer[4096];
            DWORD bytes_read;
            if (ReadFile(pipe, buffer, sizeof(buffer) - 1, &bytes_read, nullptr)) {
                buffer[bytes_read] = '\0';
                const std::string cmd(buffer);
                const std::string resp = process_command(cmd);

                DWORD written = 0;
                if (!WriteFile(pipe, resp.c_str(), (DWORD)resp.size(), &written, nullptr)) {
                    WP_WARN("Failed to write IPC response");
                }
            }
        }

        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }
}

std::string IPCServer::process_command(const std::string& cmd) {
    WP_INFO("Received IPC command: {}", cmd);
    if (m_callback) {
        try {
            auto resp = m_callback(cmd);
            if (resp.empty()) return "OK";
            return resp;
        } catch (const std::exception& e) {
            return std::string("ERR exception: ") + e.what();
        }
    }
    return "ERR no callback";
}

} // namespace wp
