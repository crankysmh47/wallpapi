#include "monitor.hpp"
#include "logger.hpp"

namespace wp {

SystemState g_system_state;

HWINEVENTHOOK Monitor::s_win_hook = nullptr;

void Monitor::init() {
    // Detect power status initially
    SYSTEM_POWER_STATUS power;
    if (GetSystemPowerStatus(&power)) {
        g_system_state.is_on_battery = (power.ACLineStatus == 0);
    }

    // Set hook for window changes
    s_win_hook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
        nullptr, win_event_proc, 0, 0, WINEVENT_OUTOFCONTEXT
    );

    WP_INFO("System monitor initialized.");
}

void Monitor::cleanup() {
    if (s_win_hook) {
        UnhookWinEvent(s_win_hook);
    }
}

void CALLBACK Monitor::win_event_proc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
    if (event == EVENT_SYSTEM_FOREGROUND) {
        // Check if foreground window is fullscreen
        RECT rect;
        if (GetWindowRect(hwnd, &rect)) {
            int w = rect.right - rect.left;
            int h = rect.bottom - rect.top;
            
            if (w >= GetSystemMetrics(SM_CXSCREEN) && h >= GetSystemMetrics(SM_CYSCREEN)) {
                // Potential fullscreen app
                char class_name[256];
                GetClassNameA(hwnd, class_name, sizeof(class_name));
                
                // Ignore desktop itself
                if (strcmp(class_name, "Progman") != 0 && strcmp(class_name, "WorkerW") != 0) {
                    WP_INFO("Fullscreen app detected: {}", class_name);
                    g_system_state.is_gaming = true;
                    return;
                }
            }
        }
        g_system_state.is_gaming = false;
    }
}

} // namespace wp
