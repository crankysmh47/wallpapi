#include "graphics.hpp"
#include "logger.hpp"

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
    const char* cmd[] = {"loadfile", path.c_str(), nullptr};
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

// --- GraphicsEngine video-related methods ---

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

void GraphicsEngine::pause_video() {
    if (m_video_player) m_video_player->pause();
}

void GraphicsEngine::resume_video() {
    if (m_video_player) m_video_player->play();
}

} // namespace wp

