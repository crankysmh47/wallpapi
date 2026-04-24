#include "graphics.hpp"
#include "logger.hpp"
#include <mpv/client.h>

namespace wp {

// --- VideoPlayer Implementation ---

VideoPlayer::VideoPlayer(HWND hwnd) {
    m_mpv = mpv_create();
    if (!m_mpv) {
        WP_ERROR("Failed to create mpv instance");
        return;
    }

    int64_t wid = (int64_t)hwnd;
    mpv_set_option(m_mpv, "wid", MPV_FORMAT_INT64, &wid);

    mpv_set_option_string(m_mpv, "config", "no");
    mpv_set_option_string(m_mpv, "idle", "yes");
    mpv_set_option_string(m_mpv, "vo", "gpu-next");
    mpv_set_option_string(m_mpv, "hwdec", "auto");
    mpv_set_option_string(m_mpv, "loop-file", "inf");
    mpv_set_option_string(m_mpv, "input-default-bindings", "no");
    mpv_set_option_string(m_mpv, "input-vo-keyboard", "no");
    
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
    RECT rect;
    GetClientRect(hwnd, &rect);
    m_width = rect.right - rect.left;
    m_height = rect.bottom - rect.top;

    m_video_player = std::make_unique<VideoPlayer>(m_hwnd);
    return true;
}

void GraphicsEngine::render() {
    // mpv handles rendering to the WID natively.
}

void GraphicsEngine::cleanup() {}

void GraphicsEngine::resize(int width, int height) {
    m_width = width;
    m_height = height;
}

void GraphicsEngine::load_video(const std::string& path) {
    if (m_video_player) {
        m_video_player->load(path);
        m_video_player->play();
    }
}

void GraphicsEngine::pause_video() {
    if (m_video_player) m_video_player->pause();
}

void GraphicsEngine::resume_video() {
    if (m_video_player) m_video_player->play();
}

} // namespace wp
