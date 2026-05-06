#include "graphics.hpp"
#include "logger.hpp"
#include "shader_renderer.hpp"
#include <mpv/client.h>

namespace wp {

// --- VideoPlayer Implementation ---

VideoPlayer::VideoPlayer(HWND hwnd, MpvRuntimeOptions options) : m_options(std::move(options)) {
    m_mpv = mpv_create();
    if (!m_mpv) {
        WP_ERROR("Failed to create mpv instance");
        return;
    }

    int64_t wid = (int64_t)hwnd;
    mpv_set_option(m_mpv, "wid", MPV_FORMAT_INT64, &wid);

    for (const auto& [k, v] : build_mpv_options(m_options)) {
        mpv_set_option_string(m_mpv, k.c_str(), v.c_str());
    }
    
    int err = mpv_initialize(m_mpv);
    if (err < 0) {
        WP_ERROR("Failed to initialize mpv: {}", mpv_error_string(err));
    }
}

VideoPlayer::~VideoPlayer() {
    if (m_mpv) mpv_terminate_destroy(m_mpv);
}

bool VideoPlayer::load(const std::string& path) {
    const char* cmd[] = { "loadfile", path.c_str(), nullptr };
    return mpv_command(m_mpv, cmd) >= 0;
}

void VideoPlayer::play() {
    int pause = 0;
    mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &pause);
}

void VideoPlayer::pause() {
    int pause = 1;
    mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &pause);
}

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

void GraphicsEngine::cleanup() {}

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

void GraphicsEngine::load_video(const std::string& path, MpvRuntimeOptions options) {
    m_shader_mode = false;
    // Recreate the player if options changed (e.g. mute toggled).
    if (!m_video_player || m_video_options.muted != options.muted) {
        m_video_player.reset();
        m_video_player = std::make_unique<VideoPlayer>(m_render_hwnd, options);
        m_video_options = options;
    }
    if (m_video_player) {
        m_video_player->load(path);
        m_video_player->play();
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

void GraphicsEngine::pause_video() {
    if (m_video_player) m_video_player->pause();
}

void GraphicsEngine::resume_video() {
    if (m_video_player) m_video_player->play();
}

void GraphicsEngine::set_mouse(float x, float y, bool clicking) {
    if (m_shader_renderer) {
        m_shader_renderer->set_mouse(x, y, clicking);
    }
}

} // namespace wp
