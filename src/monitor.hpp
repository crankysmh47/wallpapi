#pragma once
#include <windows.h>
#include <atomic>

namespace wp {

struct SystemState {
    std::atomic<bool> is_gaming{false};
    std::atomic<bool> is_on_battery{false};
    std::atomic<bool> is_paused{false};
};

extern SystemState g_system_state;

class Monitor {
public:
    static void init();
    static void cleanup();

private:
    static void CALLBACK win_event_proc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);
    static HHOOK s_hook;
    static HWINEVENTHOOK s_win_hook;
};

} // namespace wp
