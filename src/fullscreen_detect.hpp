#pragma once
#include <windows.h>

namespace wp {

// Returns true if `win` covers (at least) the monitor bounds.
inline bool is_fullscreen_on_monitor(const RECT& win, const RECT& mon) {
    const int ww = win.right - win.left;
    const int wh = win.bottom - win.top;
    const int mw = mon.right - mon.left;
    const int mh = mon.bottom - mon.top;
    if (ww < mw || wh < mh) return false;
    // Require the window to start at or above/left of the monitor and end at or beyond it.
    return win.left <= mon.left && win.top <= mon.top && win.right >= mon.right && win.bottom >= mon.bottom;
}

} // namespace wp

