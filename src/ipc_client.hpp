#pragma once
#include <windows.h>
#include <string>

namespace wp {

inline constexpr const char* kEnginePipeName = "\\\\.\\pipe\\wp_engine_pipe";

inline std::string ipc_send(const std::string& cmd, std::string* error = nullptr) {
    HANDLE pipe = CreateFileA(
        kEnginePipeName,
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr,
        OPEN_EXISTING,
        0, nullptr
    );

    if (pipe == INVALID_HANDLE_VALUE) {
        if (error) *error = "Could not connect to engine. Is wp-engine running?";
        return {};
    }

    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(pipe, &mode, nullptr, nullptr);

    DWORD bytes_written = 0;
    if (!WriteFile(pipe, cmd.c_str(), static_cast<DWORD>(cmd.size()), &bytes_written, nullptr)) {
        if (error) *error = "Failed to send command.";
        CloseHandle(pipe);
        return {};
    }

    char buffer[8192];
    DWORD bytes_read = 0;
    if (!ReadFile(pipe, buffer, sizeof(buffer) - 1, &bytes_read, nullptr)) {
        if (error) *error = "No response from engine.";
        CloseHandle(pipe);
        return {};
    }

    buffer[bytes_read] = '\0';
    CloseHandle(pipe);
    return std::string(buffer);
}

inline bool ipc_ok(const std::string& resp) {
    return !resp.empty() && resp.rfind("OK", 0) == 0;
}

} // namespace wp
