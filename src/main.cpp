#define NOMINMAX
#include "logger.hpp"
#include <windows.h>
#include "graphics.hpp"
#include "monitor.hpp"
#include "config.hpp"
#include "lua_engine.hpp"
#include "ipc.hpp"

static HWND g_workerw = nullptr;
static HWND g_wallpaper_host = nullptr;
static HWND g_msg_window = nullptr;
static wp::GraphicsEngine* g_graphics = nullptr;
static wp::ConfigManager* g_config = nullptr;
static wp::LuaEngine* g_lua = nullptr;
static wp::IPCServer* g_ipc = nullptr;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    char className[256];
    GetClassNameA(hwnd, className, sizeof(className));

    HWND p = FindWindowExA(hwnd, nullptr, "SHELLDLL_DefView", nullptr);
    if (p != nullptr) {
        // We found the window with the icons. The wallpaper WorkerW is usually its sibling.
        HWND next_workerw = FindWindowExA(nullptr, hwnd, "WorkerW", nullptr);
        if (next_workerw) {
            g_workerw = next_workerw;
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

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    // Enumerate all WorkerW windows and pick the one that covers the full screen
    g_workerw = nullptr;
    EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
        char cls[256]; GetClassNameA(hwnd, cls, 256);
        if (strcmp(cls, "WorkerW") != 0) return TRUE;
        RECT wr; GetWindowRect(hwnd, &wr);
        int w = wr.right - wr.left;
        int h = wr.bottom - wr.top;
        if (wr.left == 0 && wr.top == 0 && w == GetSystemMetrics(SM_CXSCREEN) && h == GetSystemMetrics(SM_CYSCREEN)) {
            g_workerw = hwnd;
        }
        return TRUE;
    }, 0);

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
                
                // Re-verify the desktop window because Windows might have reset it
                HWND new_host = GetWallpaperWindow(); 
                if (new_host != g_wallpaper_host) {
                    WP_INFO("Wallpaper host changed after resume. Re-initializing graphics.");
                    g_wallpaper_host = new_host;
                    if (g_graphics) {
                        g_graphics->init(g_wallpaper_host);
                        // Reload current video
                        if (g_config) g_graphics->load_video(g_config->get_current().video_path);
                    }
                }
                
                if (g_graphics) g_graphics->resume_video();
            }
            return TRUE;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    // Single Instance Enforcement
    HANDLE hMutex = CreateMutexA(nullptr, TRUE, "Wallpapi_SingleInstance_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return 0;
    }

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
        // We could convert to char** if needed, but the current logic doesn't use them.
        // We keep this here to support future CLI flag expansion.
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
        std::string video = g_config->get_current().video_path;
        if (video.empty() || !std::filesystem::exists(video)) {
            WP_WARN("Configured video '{}' not found. Searching for fallback...", video);
            if (std::filesystem::exists("wallpapers")) {
                for (const auto& entry : std::filesystem::directory_iterator("wallpapers")) {
                    if (entry.path().extension() == ".mp4") {
                        video = entry.path().string();
                        WP_INFO("Using fallback wallpaper: {}", video);
                        break;
                    }
                }
            }
        }
        g_graphics->load_video(video);
    }
    
    g_config->start_watching(config_path, [](const wp::Config& config) {
        if (g_graphics) {
            g_graphics->load_video(config.video_path);
        }
    });

    g_ipc = new wp::IPCServer();
    g_ipc->start([](const std::string& cmd) {
        if (cmd.find("set-video ") == 0) {
            std::string path = cmd.substr(10);
            if (g_graphics) g_graphics->load_video(path);
        } else if (cmd == "pause") {
            wp::g_system_state.is_paused = true;
        } else if (cmd == "resume") {
            wp::g_system_state.is_paused = false;
        }
    });

    WP_INFO("Wallpapi initialized.");
    
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            if (!wp::g_system_state.is_gaming && !wp::g_system_state.is_on_battery && !wp::g_system_state.is_paused) {
                g_graphics->render();
            } else {
                // Throttle when paused
                Sleep(16); // ~60fps check
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
