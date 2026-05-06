#include <doctest/doctest.h>

#include "ipc.hpp"

#include <windows.h>
#include <string>
#include <thread>

static std::string make_test_pipe_name() {
    return "\\\\.\\pipe\\wp_engine_pipe_test_" + std::to_string(GetCurrentProcessId());
}

static std::string send_ipc(const std::string& pipe_name, const std::string& cmd) {
    HANDLE pipe = INVALID_HANDLE_VALUE;
    // The server recreates the named pipe per connection; retry briefly.
    for (int i = 0; i < 50; ++i) {
        if (WaitNamedPipeA(pipe_name.c_str(), 50)) {
            pipe = CreateFileA(
                pipe_name.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0, nullptr,
                OPEN_EXISTING,
                0, nullptr
            );
            if (pipe != INVALID_HANDLE_VALUE) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE_MESSAGE(pipe != INVALID_HANDLE_VALUE, "Failed to connect to test pipe");

    DWORD written = 0;
    REQUIRE(WriteFile(pipe, cmd.c_str(), (DWORD)cmd.size(), &written, nullptr));

    char buf[4096];
    DWORD read = 0;
    REQUIRE(ReadFile(pipe, buf, sizeof(buf) - 1, &read, nullptr));
    buf[read] = '\0';
    CloseHandle(pipe);
    return std::string(buf);
}

TEST_CASE("IPCServer replies with OK/ERR") {
    const auto pipe_name = make_test_pipe_name();

    wp::IPCServer server(pipe_name);
    server.start([](const std::string& cmd) -> std::string {
        if (cmd == "ping") return "OK pong";
        return "ERR nope";
    });

    // Give server a moment to create the pipe.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    CHECK(send_ipc(pipe_name, "ping") == "OK pong");
    CHECK(send_ipc(pipe_name, "nope") == "ERR nope");

    server.stop();
}

