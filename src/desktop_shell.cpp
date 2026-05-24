#include "desktop_shell.hpp"
#include "logger.hpp"

#include <windows.h>
#include <vector>

namespace wp {

void destroy_stale_render_windows() {
    std::vector<HWND> stale;

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        char cls[256] = {};
        if (GetClassNameA(hwnd, cls, sizeof(cls)) == 0) return TRUE;
        if (strcmp(cls, "WallpapiRenderWindow") != 0) return TRUE;

        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        auto* list = reinterpret_cast<std::vector<HWND>*>(lParam);
        if (pid != GetCurrentProcessId()) {
            list->push_back(hwnd);
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&stale));

    for (HWND hwnd : stale) {
        WP_INFO("Destroying stale render window: {:p}", (void*)hwnd);
        DestroyWindow(hwnd);
    }

    if (!stale.empty()) {
        WP_INFO("Removed {} stale render window(s).", stale.size());
    }
}

} // namespace wp
