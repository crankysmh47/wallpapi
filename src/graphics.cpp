#include "graphics.hpp"
#include "logger.hpp"

namespace wp {

static void size_to_parent(HWND parent, int& width, int& height) {
    RECT rect{};
    if (GetClientRect(parent, &rect)) {
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
    }
    // WorkerW client area can be 0 briefly; fall back to virtual screen.
    if (width < 100 || height < 100) {
        width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    }
}

GraphicsEngine::GraphicsEngine() {}
GraphicsEngine::~GraphicsEngine() { cleanup(); }

bool GraphicsEngine::init(HWND hwnd) {
    m_hwnd = hwnd;

    HINSTANCE hInst = GetModuleHandle(nullptr);
    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = hInst;
    wc.hbrBackground = nullptr;
    wc.lpszClassName = "WallpapiRenderWindow";
    RegisterClassA(&wc);

    RECT rect{};
    GetClientRect(hwnd, &rect);
    m_width = rect.right - rect.left;
    m_height = rect.bottom - rect.top;
    size_to_parent(hwnd, m_width, m_height);

    m_render_hwnd = CreateWindowExA(
        0, "WallpapiRenderWindow", nullptr,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        0, 0, m_width, m_height,
        hwnd, nullptr, hInst, nullptr
    );

    if (!m_render_hwnd) {
        WP_ERROR("Failed to create render window");
        return false;
    }

    HWND icons_hwnd = FindWindowExA(hwnd, nullptr, "SHELLDLL_DefView", nullptr);
    if (icons_hwnd) {
        SetWindowPos(m_render_hwnd, icons_hwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    } else {
        SetWindowPos(m_render_hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    WP_INFO("Created render window: {:p} (Child of {:p}) {}x{}", (void*)m_render_hwnd, (void*)hwnd, m_width, m_height);
    return true;
}

void GraphicsEngine::cleanup() {
    release_video();

    if (m_render_hwnd && IsWindow(m_render_hwnd)) {
        DestroyWindow(m_render_hwnd);
    }
    m_render_hwnd = nullptr;
    m_video_hwnd = nullptr;

    m_hwnd = nullptr;
    m_width = 0;
    m_height = 0;
}

bool GraphicsEngine::reinit(HWND hwnd) {
    cleanup();
    return init(hwnd);
}

void GraphicsEngine::resize(int width, int height) {
    m_width = width;
    m_height = height;
    if (m_render_hwnd) {
        SetWindowPos(m_render_hwnd, nullptr, 0, 0, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void GraphicsEngine::reparent(HWND new_parent) {
    if (m_hwnd == new_parent) return;
    m_hwnd = new_parent;
    if (m_render_hwnd) {
        SetParent(m_render_hwnd, new_parent);
        WP_INFO("Re-parented render window to: {:p}", (void*)new_parent);

        RECT rect{};
        GetClientRect(new_parent, &rect);
        m_width = rect.right - rect.left;
        m_height = rect.bottom - rect.top;
        size_to_parent(new_parent, m_width, m_height);
        SetWindowPos(m_render_hwnd, nullptr, 0, 0, m_width, m_height, SWP_NOZORDER | SWP_NOACTIVATE);

        HWND icons_hwnd = FindWindowExA(new_parent, nullptr, "SHELLDLL_DefView", nullptr);
        if (icons_hwnd) {
            SetWindowPos(m_render_hwnd, icons_hwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        } else {
            SetWindowPos(m_render_hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }
}

} // namespace wp
