#pragma once
#include <windows.h>
#include <cstring>

namespace wp {

inline bool is_fullscreen_on_monitor(const RECT& win, const RECT& mon) {
    const int ww = win.right - win.left;
    const int wh = win.bottom - win.top;
    const int mw = mon.right - mon.left;
    const int mh = mon.bottom - mon.top;
    if (ww < mw || wh < mh) return false;
    return win.left <= mon.left && win.top <= mon.top && win.right >= mon.right && win.bottom >= mon.bottom;
}

// Shell / IDE windows that cover the monitor but are not exclusive fullscreen.
inline bool is_ignored_fullscreen_class(const char* class_name) {
    static const char* kIgnored[] = {
        "Progman",
        "WorkerW",
        "Shell_TrayWnd",
        "XamlExplorerHostIslandWindow",
        "Windows.UI.Core.CoreWindow",
        "ApplicationFrameWindow",
        "ForegroundStaging",
        "MultitaskingViewFrame",
        "SysListView32",
        "CabinetWClass",
        "tooltips_class32",
        "NotifyIconOverflowWindow",
    };
    for (const char* ignored : kIgnored) {
        if (strcmp(class_name, ignored) == 0) return true;
    }
    return false;
}

// True for F11/borderless fullscreen — not a normal maximized window with title bar.
inline bool is_exclusive_fullscreen(HWND hwnd) {
    const LONG style = GetWindowLong(hwnd, GWL_STYLE);
    if (style & WS_POPUP) return true;
    if (style & WS_CAPTION) return false;
    if (style & WS_THICKFRAME) return false;
    return true;
}

inline bool should_pause_for_fullscreen_window(HWND hwnd) {
    if (!hwnd || !IsWindowVisible(hwnd)) return false;

    char class_name[256] = {};
    if (GetClassNameA(hwnd, class_name, sizeof(class_name)) == 0) return false;
    if (is_ignored_fullscreen_class(class_name)) return false;

    RECT rect{};
    if (!GetWindowRect(hwnd, &rect)) return false;

    HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (!mon || !GetMonitorInfoA(mon, &mi)) return false;

    if (!is_fullscreen_on_monitor(rect, mi.rcMonitor)) return false;
    return is_exclusive_fullscreen(hwnd);
}

} // namespace wp
