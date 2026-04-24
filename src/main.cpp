#include "logger.hpp"
#include <windows.h>
#include "graphics.hpp"
#include "monitor.hpp"
#include "config.hpp"
#include "lua_engine.hpp"
#include "ipc.hpp"

static HWND g_workerw = nullptr;
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
                    wp::g_system_state.is_on_battery = (power.ACLineStatus == 0);
                    if (wp::g_system_state.is_on_battery) {
                        WP_INFO("Power cord unplugged. Pausing engine.");
                    } else {
                        WP_INFO("Power cord connected. Resuming engine.");
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

int main() {
    wp::Logger::init();
    WP_INFO("Wallpapi starting...");

    HWND wallpaper_host = GetWallpaperWindow();
    if (!wallpaper_host) return 1;

    // Register window class
    const char CLASS_NAME[] = "Wallpapi_Window";
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLASS_NAME;
    RegisterClassA(&wc);

    // Create our window
    // We bypass creating our own child window and pass the WorkerW host directly to mpv!
    // This prevents Windows from treating our child window as a fullscreen game and hiding the taskbar.
    HWND hwnd = wallpaper_host;

    g_graphics = new wp::GraphicsEngine();
    if (!g_graphics->init(hwnd)) {
        WP_ERROR("Failed to initialize graphics engine!");
        return 1;
    }

    wp::Monitor::init();

    g_lua = new wp::LuaEngine();
    g_lua->init();

    g_config = new wp::ConfigManager();
    if (g_config->load("config.toml")) {
        g_graphics->load_video(g_config->get_current().video_path);
    }
    
    g_config->start_watching("config.toml", [](const wp::Config& config) {
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

    g_ipc->stop();
    g_config->stop_watching();
    wp::Monitor::cleanup();
    
    delete g_ipc;
    delete g_lua;
    delete g_config;
    delete g_graphics;
    
    WP_INFO("Shutting down.");
    return 0;
}
