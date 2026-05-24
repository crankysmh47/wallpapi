#define NOMINMAX
#include "logger.hpp"
#include <windows.h>
#include <tlhelp32.h>
#include <filesystem>
#include "graphics.hpp"
#include "monitor.hpp"
#include "config.hpp"
#include "ipc.hpp"
#include "ipc_commands.hpp"
#include "wallpaper_scan.hpp"
#include "desktop_shell.hpp"

static HWND g_workerw = nullptr;
static HWND g_wallpaper_host = nullptr;
static HWND g_msg_window = nullptr;
static wp::GraphicsEngine* g_graphics = nullptr;
static wp::ConfigManager* g_config = nullptr;
static wp::IPCServer* g_ipc = nullptr;
static std::atomic<bool> g_pause_on_battery{true};
static std::atomic<bool> g_pause_on_fullscreen{true};
static DWORD g_main_thread_id = 0;

static const std::vector<std::string>& media_extensions() {
    static const std::vector<std::string> exts = {
        ".mp4", ".mkv", ".avi", ".webm", ".mov",
        ".jpg", ".jpeg", ".png", ".webp"
    };
    return exts;
}

static void apply_config_runtime_flags(const wp::Config& cfg) {
    g_pause_on_battery = cfg.pause_on_battery;
    g_pause_on_fullscreen = cfg.pause_on_fullscreen;
}

static void reload_current_wallpaper() {
    if (!g_graphics || !g_config) return;
    const auto& cfg = g_config->get_current();
    g_graphics->load_video(cfg.video_path, wp::VideoOptions{ .muted = cfg.muted });
}

static std::string resolve_startup_video() {
    if (!g_config) return {};
    std::string video = g_config->get_current().video_path;
    if (!video.empty() && std::filesystem::exists(video)) {
        return video;
    }
    if (std::filesystem::exists("wallpapers")) {
        const auto pick = wp::find_first_by_extension("wallpapers", media_extensions());
        if (pick.has_value()) {
            return pick->string();
        }
    }
    return {};
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    HWND p = FindWindowExA(hwnd, nullptr, "SHELLDLL_DefView", nullptr);
    if (p != nullptr) {
        HWND next_workerw = FindWindowExA(nullptr, hwnd, "WorkerW", nullptr);
        if (next_workerw) {
            g_workerw = next_workerw;
            return FALSE;
        }

        HWND parent = GetParent(p);
        char parent_cls[256];
        GetClassNameA(parent, parent_cls, 256);
        if (strcmp(parent_cls, "WorkerW") == 0) {
            g_workerw = parent;
            return FALSE;
        }
    }
    return TRUE;
}

HWND GetWallpaperWindow() {
    HWND progman = FindWindowA("Progman", nullptr);

    SendMessageTimeoutA(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, nullptr);
    Sleep(500);

    g_workerw = nullptr;
    EnumWindows(EnumWindowsProc, 0);

    if (g_workerw == nullptr) {
        EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
            char cls[256]; GetClassNameA(hwnd, cls, 256);
            if (strcmp(cls, "WorkerW") == 0) {
                HWND defView = FindWindowExA(hwnd, nullptr, "SHELLDLL_DefView", nullptr);
                if (defView) {
                    g_workerw = FindWindowExA(nullptr, hwnd, "WorkerW", nullptr);
                    if (g_workerw) return FALSE;
                }
            }
            return TRUE;
        }, 0);
    }

    if (g_workerw == nullptr) {
        EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
            char cls[256]; GetClassNameA(hwnd, cls, 256);
            if (strcmp(cls, "WorkerW") == 0) {
                RECT wr; GetWindowRect(hwnd, &wr);
                int w = wr.right - wr.left;
                int h = wr.bottom - wr.top;
                if (w >= 1000 && h >= 600) {
                    g_workerw = hwnd;
                    return FALSE;
                }
            }
            return TRUE;
        }, 0);
    }

    if (g_workerw == nullptr) {
        g_workerw = FindWindowExA(progman, nullptr, "WorkerW", nullptr);
    }

    if (g_workerw == nullptr) {
        WP_WARN("Desktop hijack failed. Falling back to Progman.");
        g_workerw = progman;
    }

    WP_INFO("Attached to desktop host: {:p}", (void*)g_workerw);
    return g_workerw;
}

static void recover_after_wake() {
    WP_INFO("Recovering wallpaper after wake...");
    wp::destroy_stale_render_windows();

    HWND new_host = nullptr;
    for (int attempt = 0; attempt < 15 && !new_host; ++attempt) {
        new_host = GetWallpaperWindow();
        if (!new_host) {
            Sleep(200);
        }
    }

    if (!new_host) {
        WP_ERROR("Failed to re-attach to desktop after wake.");
        return;
    }

    g_wallpaper_host = new_host;
    if (g_graphics) {
        g_graphics->reinit(g_wallpaper_host);
        reload_current_wallpaper();
    }

    // Return unused pages to the OS after rebuilding the player.
    SetProcessWorkingSetSize(GetCurrentProcess(), static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1));
    WP_INFO("Wake recovery complete.");
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_SIZE:
            if (g_graphics) {
                g_graphics->resize(LOWORD(lParam), HIWORD(lParam));
            }
            return 0;
        case WM_POWERBROADCAST:
            if (wParam == PBT_APMPOWERSTATUSCHANGE) {
                SYSTEM_POWER_STATUS power;
                if (GetSystemPowerStatus(&power)) {
                    bool on_battery = (power.ACLineStatus == 0);
                    if (on_battery != wp::g_system_state.is_on_battery) {
                        wp::g_system_state.is_on_battery = on_battery;
                        if (!g_pause_on_battery) {
                            return TRUE;
                        }
                        if (on_battery) {
                            WP_INFO("Power cord unplugged. Pausing video.");
                            if (g_graphics) g_graphics->pause_video();
                        } else {
                            WP_INFO("Power cord connected. Resuming video.");
                            if (g_graphics) g_graphics->resume_video();
                        }
                    }
                }
            } else if (wParam == PBT_APMSUSPEND) {
                WP_INFO("System suspending. Releasing video resources.");
                wp::g_system_state.is_paused = true;
                if (g_graphics) g_graphics->release_video();
            } else if (wParam == PBT_APMRESUMEAUTOMATIC || wParam == PBT_APMRESUMESUSPEND) {
                wp::g_system_state.is_paused = false;
                recover_after_wake();
            }
            return TRUE;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    g_main_thread_id = GetCurrentThreadId();

    HANDLE hMutex = CreateMutexA(nullptr, TRUE, "Wallpapi_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        bool engine_running = false;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe{};
            pe.dwSize = sizeof(pe);
            if (Process32First(snap, &pe)) {
                do {
                    if (_stricmp(pe.szExeFile, "wp-engine.exe") == 0) {
                        engine_running = true;
                        break;
                    }
                } while (Process32Next(snap, &pe));
            }
            CloseHandle(snap);
        }
        if (engine_running) {
            WP_INFO("Another wp-engine instance is already running. Exiting.");
            if (hMutex) CloseHandle(hMutex);
            return 0;
        }
        WP_WARN("Stale single-instance mutex detected. Continuing startup.");
    }

    SetProcessDPIAware();
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (!wp::media_foundation_startup()) {
        WP_ERROR("Failed to start Media Foundation.");
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }

    wp::Logger::init();
    WP_INFO("Wallpapi starting...");

    char exe_path[MAX_PATH];
    GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
    std::filesystem::path p(exe_path);
    std::filesystem::current_path(p.parent_path());
    WP_INFO("Working directory set to: {}", std::filesystem::current_path().string());

    HWND wallpaper_host = nullptr;
    int retries = 0;
    int retry_delay = 100;
    while (!wallpaper_host && retries < 20) {
        wallpaper_host = GetWallpaperWindow();
        if (!wallpaper_host) {
            WP_WARN("Desktop shell not ready yet. Retrying in {}ms... (Attempt {}/20)", retry_delay, retries + 1);
            Sleep(retry_delay);
            retries++;
            retry_delay = std::min(retry_delay * 2, 2000);
        }
    }

    if (!wallpaper_host) {
        WP_ERROR("Failed to attach to desktop after 20 attempts. Exiting.");
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }

    const char CLASS_NAME[] = "Wallpapi_Window";
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassA(&wc);

    g_msg_window = CreateWindowExA(0, CLASS_NAME, "Wallpapi Controller", 0, 0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
    if (!g_msg_window) {
        WP_ERROR("Failed to create message window!");
    }

    g_wallpaper_host = wallpaper_host;
    wp::destroy_stale_render_windows();

    g_graphics = new wp::GraphicsEngine();
    if (!g_graphics->init(g_wallpaper_host)) {
        WP_ERROR("Failed to initialize graphics engine!");
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }

    wp::Monitor::init();

    std::string config_path = std::filesystem::absolute("config.toml").string();
    g_config = new wp::ConfigManager();
    if (g_config->load(config_path)) {
        apply_config_runtime_flags(g_config->get_current());
        const auto video = resolve_startup_video();
        if (!video.empty()) {
            g_graphics->load_video(video, wp::VideoOptions{ .muted = g_config->get_current().muted });
            g_config->set_current_video(video);
        }
    }

    g_config->start_watching(config_path, [](const wp::Config& config) {
        if (!g_graphics) return;
        apply_config_runtime_flags(config);
        if (config.video_path.empty() || !std::filesystem::exists(config.video_path)) {
            return;
        }
        g_graphics->load_video(config.video_path, wp::VideoOptions{ .muted = config.muted });
    });

    g_ipc = new wp::IPCServer();
    g_ipc->start([](const std::string& cmd) -> std::string {
        const auto parsed = wp::parse_ipc_command(cmd);
        switch (parsed.type) {
            case wp::IPCCommandType::SetVideo:
            case wp::IPCCommandType::SetImage: {
                if (!g_graphics) return "ERR engine-not-ready";
                const auto opts = wp::VideoOptions{ .muted = g_config ? g_config->get_current().muted : true };
                g_graphics->load_video(parsed.argument, opts);
                if (g_config && !g_config->set_current_video(parsed.argument)) {
                    return "ERR file-not-found";
                }
                return "OK set-video";
            }
            case wp::IPCCommandType::Pause:
                wp::g_system_state.is_paused = true;
                if (g_graphics) g_graphics->pause_video();
                return "OK pause";
            case wp::IPCCommandType::Resume:
                wp::g_system_state.is_paused = false;
                if (g_graphics) g_graphics->resume_video();
                return "OK resume";
            case wp::IPCCommandType::GetStatus: {
                const std::string paused = wp::g_system_state.is_paused.load() ? "true" : "false";
                const std::string on_battery = wp::g_system_state.is_on_battery.load() ? "true" : "false";
                const std::string fullscreen = wp::g_system_state.is_gaming.load() ? "true" : "false";
                std::string path;
                if (g_config) {
                    path = g_config->get_current().video_path;
                }
                return "OK status mode=media paused=" + paused + " on_battery=" + on_battery +
                       " fullscreen=" + fullscreen + " path=" + path;
            }
            case wp::IPCCommandType::ListWallpapers: {
                const auto list = wp::list_wallpapers(media_extensions());
                std::string resp = "OK wallpapers\n";
                for (const auto& entry : list) {
                    resp += entry.string();
                    resp += "\n";
                }
                return resp;
            }
            case wp::IPCCommandType::GetConfig:
                return format_config_status();
            case wp::IPCCommandType::SetConfig: {
                if (!g_config) return "ERR no-config";
                bool value = false;
                if (!wp::parse_bool(parsed.second_argument, &value)) {
                    return "ERR invalid-bool";
                }
                const auto& key = parsed.argument;
                bool ok = false;
                if (key == "pause_on_battery") {
                    ok = g_config->set_pause_on_battery(value);
                    if (ok) apply_config_runtime_flags(g_config->get_current());
                } else if (key == "pause_on_fullscreen") {
                    ok = g_config->set_pause_on_fullscreen(value);
                    if (ok) apply_config_runtime_flags(g_config->get_current());
                } else if (key == "muted") {
                    ok = g_config->set_muted(value);
                    if (ok && g_graphics) {
                        reload_current_wallpaper();
                    }
                } else if (key == "video") {
                    return apply_wallpaper_path(parsed.second_argument);
                } else {
                    return "ERR unknown-config-key";
                }
                return ok ? ("OK config " + key + "=" + (value ? "true" : "false")) : "ERR config-save-failed";
            }
            case wp::IPCCommandType::TogglePauseOnBattery: {
                if (!g_config) return "ERR no-config";
                const bool next = !g_config->get_current().pause_on_battery;
                if (!g_config->set_pause_on_battery(next)) return "ERR config-save-failed";
                apply_config_runtime_flags(g_config->get_current());
                return std::string("OK pause_on_battery=") + (next ? "true" : "false");
            }
            case wp::IPCCommandType::TogglePauseOnFullscreen: {
                if (!g_config) return "ERR no-config";
                const bool next = !g_config->get_current().pause_on_fullscreen;
                if (!g_config->set_pause_on_fullscreen(next)) return "ERR config-save-failed";
                apply_config_runtime_flags(g_config->get_current());
                return std::string("OK pause_on_fullscreen=") + (next ? "true" : "false");
            }
            case wp::IPCCommandType::ToggleMuted: {
                if (!g_config) return "ERR no-config";
                const bool next = !g_config->get_current().muted;
                if (!g_config->set_muted(next)) return "ERR config-save-failed";
                reload_current_wallpaper();
                return std::string("OK muted=") + (next ? "true" : "false");
            }
            case wp::IPCCommandType::OpenWallpapers:
                return wp::open_wallpapers_folder() ? "OK open-wallpapers" : "ERR open-failed";
            case wp::IPCCommandType::AddWallpaper: {
                const auto copied = wp::copy_into_wallpapers(parsed.argument);
                if (copied.empty()) return "ERR add-failed";
                return apply_wallpaper_path(copied);
            }
            case wp::IPCCommandType::NextWallpaper: {
                std::filesystem::path current;
                if (g_config) current = g_config->get_current().video_path;
                const auto next = wp::pick_next_wallpaper(current, media_extensions());
                if (!next) return "ERR no-wallpapers";
                return apply_wallpaper_path(next->string());
            }
            case wp::IPCCommandType::RandomWallpaper: {
                const auto pick = wp::pick_random_wallpaper(media_extensions());
                if (!pick) return "ERR no-wallpapers";
                return apply_wallpaper_path(pick->string());
            }
            case wp::IPCCommandType::Help:
                return "OK help\n"
                       "set <path> | pause | resume | status | list | config get\n"
                       "config set <key> <true|false>  (pause_on_battery, pause_on_fullscreen, muted, video)\n"
                       "toggle pause_on_battery | toggle pause_on_fullscreen | toggle muted\n"
                       "open | add <path> | next | random | stop";
            case wp::IPCCommandType::Stop:
                PostThreadMessageA(g_main_thread_id, WM_QUIT, 0, 0);
                return "OK stop";
            case wp::IPCCommandType::Unknown:
            default:
                WP_WARN("Unknown IPC command: '{}'", cmd);
                return "ERR unknown-command";
        }
    });

    WP_INFO("Wallpapi initialized.");

    MSG msg = {};
    bool was_playing = false;
    while (msg.message != WM_QUIT) {
        const bool fullscreen_block = (g_pause_on_fullscreen && wp::g_system_state.is_gaming);
        const bool battery_block = (g_pause_on_battery && wp::g_system_state.is_on_battery);
        const bool should_play = (!fullscreen_block && !battery_block && !wp::g_system_state.is_paused);

        if (should_play != was_playing) {
            was_playing = should_play;
            if (g_graphics) {
                if (should_play) g_graphics->resume_video();
                else g_graphics->pause_video();
            }
        }

        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            WaitMessage();
        }
    }

    if (g_msg_window) DestroyWindow(g_msg_window);

    g_ipc->stop();
    g_config->stop_watching();
    wp::Monitor::cleanup();

    delete g_ipc;
    delete g_config;
    delete g_graphics;

    WP_INFO("Shutting down.");

    wp::media_foundation_shutdown();
    CoUninitialize();

    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    return 0;
}
