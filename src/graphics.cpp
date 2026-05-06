#include "graphics.hpp"
#include "logger.hpp"
#include "shader_renderer.hpp"

namespace wp {

// --- GraphicsEngine Implementation ---

GraphicsEngine::GraphicsEngine() {}
GraphicsEngine::~GraphicsEngine() { cleanup(); }

bool GraphicsEngine::init(HWND hwnd) {
    m_hwnd = hwnd;
    
    // Create a child window for rendering
    HINSTANCE hInst = GetModuleHandle(nullptr);
    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = hInst;
    wc.hbrBackground = nullptr;
    wc.lpszClassName = "WallpapiRenderWindow";
    RegisterClassA(&wc);

    HWND target_parent = hwnd; 

    RECT rect;
    GetClientRect(target_parent, &rect);
    m_width = rect.right - rect.left;
    m_height = rect.bottom - rect.top;

    m_render_hwnd = CreateWindowExA(
        0, "WallpapiRenderWindow", nullptr,
        WS_CHILD | WS_VISIBLE,
        0, 0, m_width, m_height,
        target_parent, nullptr, hInst, nullptr
    );

    if (!m_render_hwnd) {
        WP_ERROR("Failed to create render window");
        return false;
    }

    HWND icons_hwnd = FindWindowExA(target_parent, nullptr, "SHELLDLL_DefView", nullptr);
    if (icons_hwnd) {
        SetWindowPos(m_render_hwnd, icons_hwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    } else {
        SetWindowPos(m_render_hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    WP_INFO("Created render window: {:p} (Child of {:p})", (void*)m_render_hwnd, (void*)hwnd);

    return true;
}

void GraphicsEngine::render() {
    if (m_shader_mode && m_shader_renderer) {
        m_shader_renderer->render();
    }
}

void GraphicsEngine::cleanup() {
    // Tear down in a safe order:
    // 1) Stop renderers/players that may be using the window/device.
    m_shader_mode = false;
    m_shader_renderer.reset();
    m_video_player.reset();

    // 2) Destroy the render window.
    if (m_render_hwnd && IsWindow(m_render_hwnd)) {
        DestroyWindow(m_render_hwnd);
    }
    m_render_hwnd = nullptr;

    // 3) Reset bookkeeping.
    m_hwnd = nullptr;
    m_width = 0;
    m_height = 0;
    m_video_options = MpvRuntimeOptions{};
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
    if (m_shader_renderer) {
        m_shader_renderer->resize(width, height);
    }
}

void GraphicsEngine::reparent(HWND new_parent) {
    if (m_hwnd == new_parent) return;
    m_hwnd = new_parent;
    if (m_render_hwnd) {
        SetParent(m_render_hwnd, new_parent);
        WP_INFO("Re-parented render window to: {:p}", (void*)new_parent);

        RECT rect;
        GetClientRect(new_parent, &rect);
        resize(rect.right - rect.left, rect.bottom - rect.top);

        HWND icons_hwnd = FindWindowExA(new_parent, nullptr, "SHELLDLL_DefView", nullptr);
        if (icons_hwnd) {
            SetWindowPos(m_render_hwnd, icons_hwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        } else {
            SetWindowPos(m_render_hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }
}

bool GraphicsEngine::load_shader(const std::string& path) {
    if (!m_shader_renderer) {
        m_shader_renderer = std::make_unique<ShaderRenderer>();
        if (!m_shader_renderer->init(m_render_hwnd, m_width, m_height)) {
            m_shader_renderer.reset();
            return false;
        }
    }
    
    if (m_shader_renderer->load_shader(path)) {
        m_shader_mode = true;
        if (m_video_player) m_video_player->pause();
        return true;
    }
    return false;
}

void GraphicsEngine::set_mouse(float x, float y, bool clicking) {
    if (m_shader_renderer) {
        m_shader_renderer->set_mouse(x, y, clicking);
    }
}

} // namespace wp
