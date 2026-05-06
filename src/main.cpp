#define NOMINMAX
#include "logger.hpp"
#include <windows.h>
#include "graphics.hpp"
#include "monitor.hpp"
#include "config.hpp"
#include "lua_engine.hpp"
#include "ipc.hpp"
#include "ipc_commands.hpp"
#include "wallpaper_scan.hpp"
#include "fps_limiter.hpp"

static HWND g_workerw = nullptr;
static HWND g_wallpaper_host = nullptr;
static HWND g_msg_window = nullptr;
static wp::GraphicsEngine* g_graphics = nullptr;
static wp::ConfigManager* g_config = nullptr;
static wp::LuaEngine* g_lua = nullptr;
static wp::IPCServer* g_ipc = nullptr;
static std::atomic<bool> g_pause_on_battery{true};
static std::atomic<bool> g_pause_on_fullscreen{true};
static DWORD g_main_thread_id = 0;

static void apply_config_runtime_flags(const wp::Config& cfg) {
    g_pause_on_battery = cfg.pause_on_battery;
    g_pause_on_fullscreen = cfg.pause_on_fullscreen;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    HWND p = FindWindowExA(hwnd, nullptr, "SHELLDLL_DefView", nullptr);
    if (p != nullptr) {
        // Method A: Sibling search
        HWND next_workerw = FindWindowExA(nullptr, hwnd, "WorkerW", nullptr);
        if (next_workerw) {
            g_workerw = next_workerw;
            return FALSE;
        }
        
        // Method B: The parent might be the WorkerW
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
    
    // Create the WorkerW split
    SendMessageTimeoutA(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, nullptr);
    Sleep(500);

    g_workerw = nullptr;

    // Method 1: Sibling search - find WorkerW next to the one holding SHELLDLL_DefView
    EnumWindows(EnumWindowsProc, 0);

    // Method 2: Enumerate all WorkerW windows and find the one with SHELLDLL_DefView
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

    // Method 3: Fullscreen search - find any WorkerW large enough to be a desktop layer
    if (g_workerw == nullptr) {
        EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
            char cls[256]; GetClassNameA(hwnd, cls, 256);
            if (strcmp(cls, "WorkerW") == 0) {
                RECT wr; GetWindowRect(hwnd, &wr);
                int w = wr.right - wr.left;
                int h = wr.bottom - wr.top;
                WP_INFO("Found WorkerW: {}x{} at ({},{})", w, h, wr.left, wr.top);
                if (w >= 1000 && h >= 600) {
                    g_workerw = hwnd;
                    return FALSE;
                }
            }
            return TRUE;
        }, 0);
    }

    // Method 4: Child of Progman
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
                            // Respect config: no auto-pause on battery.
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
                WP_INFO("System suspending. Pausing engine.");
                wp::g_system_state.is_paused = true;
                if (g_graphics) g_graphics->pause_video();
            } else if (wParam == PBT_APMRESUMEAUTOMATIC || wParam == PBT_APMRESUMESUSPEND) {
                WP_INFO("System resuming. Checking desktop hijack...");
                wp::g_system_state.is_paused = false;

                // Re-verify the desktop window because Windows might have reset it after sleep
                HWND new_host = GetWallpaperWindow();
                if (new_host != g_wallpaper_host) {
                    WP_INFO("Wallpaper host changed after resume. Re-initializing graphics.");
                    g_wallpaper_host = new_host;
                    if (g_graphics) {
                        g_graphics->reinit(g_wallpaper_host);
                        if (g_config) {
                            if (!g_config->get_current().shader_path.empty()) {
                                g_graphics->load_shader(g_config->get_current().shader_path);
                            } else {
                                g_graphics->load_video(
                                    g_config->get_current().video_path,
                                    wp::MpvRuntimeOptions{ .muted = g_config->get_current().muted }
                                );
                            }
                        }
                    }
                } else {
                    if (g_graphics) {
                        g_graphics->reparent(g_workerw);
                        g_graphics->resume_video();
                    }
                }
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
    // Single Instance Enforcement
    HANDLE hMutex = CreateMutexA(nullptr, TRUE, "Wallpapi_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

    // Enable High DPI Awareness
    SetProcessDPIAware();

    wp::Logger::init();
    WP_INFO("Wallpapi starting (Discreet Mode)...");

    // Set working directory to executable directory to ensure resources are found
    char exe_path[MAX_PATH];
    GetModuleFileNameA(nullptr, exe_path, MAX_PATH);
    std::filesystem::path p(exe_path);
    std::filesystem::current_path(p.parent_path());
    WP_INFO("Working directory set to: {}", std::filesystem::current_path().string());

    // Command-line argument parsing for WinMain
    int argc;
    LPWSTR* argvw = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argvw) {
        LocalFree(argvw);
    }

    HWND wallpaper_host = nullptr;
    int retries = 0;
    int retry_delay = 100; // Start with 100ms for fast load
    while (!wallpaper_host && retries < 20) {
        wallpaper_host = GetWallpaperWindow();
        if (!wallpaper_host) {
            WP_WARN("Desktop shell not ready yet. Retrying in {}ms... (Attempt {}/20)", retry_delay, retries + 1);
            Sleep(retry_delay);
            retries++;
            retry_delay = std::min(retry_delay * 2, 2000); // Exponential backoff up to 2s
        }
    }

    if (!wallpaper_host) {
        WP_ERROR("Failed to attach to desktop after 20 attempts. Exiting.");
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }

    // Register window class
    const char CLASS_NAME[] = "Wallpapi_Window";
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassA(&wc);

    // Create a hidden window to receive system messages (like WM_POWERBROADCAST)
    g_msg_window = CreateWindowExA(0, CLASS_NAME, "Wallpapi Controller", 0, 0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
    if (!g_msg_window) {
        WP_ERROR("Failed to create message window!");
    }

    g_wallpaper_host = wallpaper_host;
    g_graphics = new wp::GraphicsEngine();
    if (!g_graphics->init(g_wallpaper_host)) {
        WP_ERROR("Failed to initialize graphics engine!");
        if (hMutex) CloseHandle(hMutex);
        return 1;
    }

    wp::Monitor::init();

    g_lua = new wp::LuaEngine();
    g_lua->init();

    std::string config_path = std::filesystem::absolute("config.toml").string();
    g_config = new wp::ConfigManager();
    if (g_config->load(config_path)) {
        apply_config_runtime_flags(g_config->get_current());
        if (!g_config->get_current().shader_path.empty()) {
            g_graphics->load_shader(g_config->get_current().shader_path);
        } else {
            std::string video = g_config->get_current().video_path;
            if (video.empty() || !std::filesystem::exists(video)) {
                if (std::filesystem::exists("wallpapers")) {
                    const auto pick = wp::find_first_by_extension(
                        "wallpapers",
                        {".mp4", ".mkv", ".avi", ".webm", ".mov", ".jpg", ".jpeg", ".png", ".webp"}
                    );
                    if (pick.has_value()) {
                        video = pick->string();
                    }
                }
            }
            g_graphics->load_video(video, wp::MpvRuntimeOptions{ .muted = g_config->get_current().muted });
        }
    }
    
    g_config->start_watching(config_path, [](const wp::Config& config) {
        if (g_graphics) {
            apply_config_runtime_flags(config);
            if (!config.shader_path.empty()) {
                g_graphics->load_shader(config.shader_path);
            } else {
                g_graphics->load_video(config.video_path, wp::MpvRuntimeOptions{ .muted = config.muted });
            }
        }
    });

    g_ipc = new wp::IPCServer();
    g_ipc->start([](const std::string& cmd) -> std::string {
        const auto parsed = wp::parse_ipc_command(cmd);
        switch (parsed.type) {
            case wp::IPCCommandType::SetVideo:
                if (g_graphics) g_graphics->load_video(parsed.argument, wp::MpvRuntimeOptions{ .muted = g_config ? g_config->get_current().muted : true });
                if (g_config) g_config->set_current_video(parsed.argument);
                return "OK set-video";
            case wp::IPCCommandType::SetImage:
                if (g_graphics) g_graphics->load_video(parsed.argument, wp::MpvRuntimeOptions{ .muted = g_config ? g_config->get_current().muted : true });
                if (g_config) g_config->set_current_video(parsed.argument);
                return "OK set-image";
            case wp::IPCCommandType::SetShader:
                if (g_graphics) g_graphics->load_shader(parsed.argument);
                if (g_config) g_config->set_current_shader(parsed.argument);
                return "OK set-shader";
            case wp::IPCCommandType::Pause:
                wp::g_system_state.is_paused = true;
                if (g_graphics) g_graphics->pause_video();
                return "OK pause";
            case wp::IPCCommandType::Resume:
                wp::g_system_state.is_paused = false;
                if (g_graphics) g_graphics->resume_video();
                return "OK resume";
            case wp::IPCCommandType::GetStatus: {
                const auto paused = wp::g_system_state.is_paused.load() ? "true" : "false";
                const auto on_battery = wp::g_system_state.is_on_battery.load() ? "true" : "false";
                const auto fullscreen = wp::g_system_state.is_gaming.load() ? "true" : "false";

                std::string mode = "none";
                std::string path;
                if (g_config) {
                    if (!g_config->get_current().shader_path.empty()) {
                        mode = "shader";
                        path = g_config->get_current().shader_path;
                    } else if (!g_config->get_current().video_path.empty()) {
                        mode = "media";
                        path = g_config->get_current().video_path;
                    }
                }

                return "OK status mode=" + mode + " paused=" + paused + " on_battery=" + on_battery +
                       " fullscreen=" + fullscreen + " path=" + path;
            }
            case wp::IPCCommandType::ListWallpapers: {
                const auto list = wp::list_files_by_extension(
                    "wallpapers",
                    {".mp4", ".mkv", ".avi", ".webm", ".mov", ".jpg", ".jpeg", ".png", ".webp", ".gif"}
                );
                std::string resp = "OK wallpapers\n";
                for (const auto& p : list) {
                    resp += p.string();
                    resp += "\n";
                }
                return resp;
            }
            case wp::IPCCommandType::Stop:
                // Ask the main message loop to exit.
                PostThreadMessageA(g_main_thread_id, WM_QUIT, 0, 0);
                return "OK stop";
            case wp::IPCCommandType::Unknown:
            default:
                WP_WARN("Unknown IPC command: '{}'", cmd);
                return "ERR unknown-command";
        }
    });

    WP_INFO("Wallpapi initialized.");
    
    // Initial render to avoid black screen if starting paused
    g_graphics->render();

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        const bool fullscreen_block = (g_pause_on_fullscreen && wp::g_system_state.is_gaming);
        const bool battery_block = (g_pause_on_battery && wp::g_system_state.is_on_battery);
        bool has_work = (!fullscreen_block && !battery_block && !wp::g_system_state.is_paused);
        
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            if (has_work) {
                const auto frame_start = std::chrono::steady_clock::now();
                // Update mouse position for shader interaction
                POINT mp;
                if (GetCursorPos(&mp)) {
                    ScreenToClient(wallpaper_host, &mp);
                    bool clicking = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
                    g_graphics->set_mouse((float)mp.x, (float)mp.y, clicking);
                }
                g_graphics->render();
                const auto frame_end = std::chrono::steady_clock::now();
                const int fps_limit = g_config ? g_config->get_current().fps_limit : 0;
                const auto sleep_for = wp::compute_frame_sleep_ms(fps_limit, frame_start, frame_end);
                if (sleep_for.count() > 0) {
                    Sleep((DWORD)sleep_for.count());
                } else {
                    Sleep(1);
                }
            } else {
                WaitMessage();
            }
        }
    }

    if (g_msg_window) DestroyWindow(g_msg_window);

    g_ipc->stop();
    g_config->stop_watching();
    wp::Monitor::cleanup();
    
    delete g_ipc;
    delete g_lua;
    delete g_config;
    delete g_graphics;
    
    WP_INFO("Shutting down.");
    
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    return 0;
}
