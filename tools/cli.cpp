#include "../src/ipc_client.hpp"
#include <iostream>
#include <string>
#include <vector>

static void print_usage() {
    std::cout <<
        "Wallpapi CLI\n"
        "Usage: wp-cli <command> [args...]\n\n"
        "Wallpaper:\n"
        "  set <path>              Set video or image wallpaper\n"
        "  list                    List files in wallpapers/\n"
        "  next                    Cycle to next wallpaper\n"
        "  random                  Pick a random wallpaper\n"
        "  add <path>              Copy file into wallpapers/ and apply\n"
        "  open                    Open wallpapers folder in Explorer\n\n"
        "Playback:\n"
        "  pause                   Pause playback\n"
        "  resume                  Resume playback\n"
        "  status                  Engine and playback status\n\n"
        "Power & fullscreen:\n"
        "  config get              Show config values\n"
        "  config set <key> <bool> pause_on_battery | pause_on_fullscreen | muted\n"
        "  toggle pause_on_battery\n"
        "  toggle pause_on_fullscreen\n"
        "  toggle muted\n\n"
        "Engine:\n"
        "  stop                    Shut down wp-engine\n"
        "  help                    Show engine command help\n";
}

static std::string join_args(int argc, char* argv[], int start) {
    std::string out;
    for (int i = start; i < argc; ++i) {
        if (!out.empty()) out += ' ';
        out += argv[i];
    }
    return out;
}

static std::string build_command(int argc, char* argv[]) {
    if (argc < 2) return {};
    const std::string cmd = argv[1];

    if (cmd == "status") return "status";
    if (cmd == "list") return "list";
    if (cmd == "pause") return "pause";
    if (cmd == "resume") return "resume";
    if (cmd == "stop" || cmd == "quit") return "stop";
    if (cmd == "open") return "open";
    if (cmd == "next") return "next";
    if (cmd == "random") return "random";

    if (cmd == "set" || cmd == "set-video" || cmd == "set-image") {
        const auto path = join_args(argc, argv, 2);
        if (path.empty()) return {};
        return cmd + " " + path;
    }
    if (cmd == "add") {
        const auto path = join_args(argc, argv, 2);
        if (path.empty()) return {};
        return "add " + path;
    }
    if (cmd == "config") {
        if (argc < 3) return {};
        const std::string sub = argv[2];
        if (sub == "get") return "config get";
        if (sub == "set" && argc >= 5) {
            return std::string("config set ") + argv[3] + " " + argv[4];
        }
        return {};
    }
    if (cmd == "toggle" && argc >= 3) {
        return std::string("toggle ") + argv[2];
    }

    return join_args(argc, argv, 1);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const std::string cmd = argv[1];
    if (cmd == "--help" || cmd == "-h" || cmd == "help") {
        print_usage();
        return 0;
    }

    const std::string ipc_cmd = build_command(argc, argv);
    if (ipc_cmd.empty()) {
        std::cerr << "Error: Invalid or incomplete command.\n\n";
        print_usage();
        return 1;
    }

    std::string error;
    const std::string resp = wp::ipc_send(ipc_cmd, &error);
    if (resp.empty()) {
        std::cerr << "Error: " << error << std::endl;
        return 1;
    }

    if (resp.rfind("OK wallpapers", 0) == 0) {
        const auto nl = resp.find('\n');
        if (nl != std::string::npos && nl + 1 < resp.size()) {
            std::cout << resp.substr(nl + 1);
        } else {
            std::cout << "(no wallpapers found)\n";
        }
        return 0;
    }

    if (resp.rfind("OK help", 0) == 0) {
        const auto nl = resp.find('\n');
        if (nl != std::string::npos) {
            std::cout << resp.substr(nl + 1) << std::endl;
        }
        return 0;
    }

    std::cout << resp << std::endl;
    return wp::ipc_ok(resp) ? 0 : 2;
}
