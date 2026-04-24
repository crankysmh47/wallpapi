#include <iostream>
#include <windows.h>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: wp-cli <command> [args...]" << std::endl;
        std::cout << "Example: wp-cli set-video \"wallpapers/video.mp4\"" << std::endl;
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

    std::cout << "Command sent successfully: " << cmd << std::endl;

    CloseHandle(pipe);
    return 0;
}
