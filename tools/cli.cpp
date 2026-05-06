#include <iostream>
#include <windows.h>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: wp-cli <command> [args...]" << std::endl;
        std::cout << "Example: wp-cli set-video \"wallpapers/video.mp4\"" << std::endl;
        std::cout << "Other: wp-cli status | wp-cli list | wp-cli stop" << std::endl;
        return 1;
    }

    std::string cmd;
    for (int i = 1; i < argc; ++i) {
        cmd += argv[i];
        if (i < argc - 1) cmd += " ";
    }

    const char* pipe_name = "\\\\.\\pipe\\wp_engine_pipe";
    
    HANDLE pipe = CreateFileA(
        pipe_name,
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr,
        OPEN_EXISTING,
        0, nullptr
    );

    if (pipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Could not connect to engine pipe. Is the engine running?" << std::endl;
        return 1;
    }

    DWORD bytes_written;
    if (!WriteFile(pipe, cmd.c_str(), (DWORD)cmd.length(), &bytes_written, nullptr)) {
        std::cerr << "Error: Failed to send command." << std::endl;
        CloseHandle(pipe);
        return 1;
    }

    // Read a short response (server returns OK/ERR text).
    char buffer[4096];
    DWORD bytes_read = 0;
    if (ReadFile(pipe, buffer, sizeof(buffer) - 1, &bytes_read, nullptr)) {
        buffer[bytes_read] = '\0';
        std::string resp(buffer);
        std::cout << resp << std::endl;
        CloseHandle(pipe);
        return (resp.rfind("OK", 0) == 0) ? 0 : 2;
    }

    std::cerr << "Error: No response from engine." << std::endl;

    CloseHandle(pipe);
    return 3;
}
