#pragma once
#include <windows.h>
#include <memory>
#include <string>

// mpv headers will be included in the implementation
struct mpv_handle;
namespace wp { class ShaderRenderer; }
#include "mpv_options.hpp"

namespace wp {

class VideoPlayer {
public:
    VideoPlayer(HWND hwnd, MpvRuntimeOptions options);
    ~VideoPlayer();

    bool load(const std::string& path);
    void play();
    void pause();

private:
    MpvRuntimeOptions m_options;
    mpv_handle* m_mpv = nullptr;
};

class GraphicsEngine {
public:
    GraphicsEngine();
    ~GraphicsEngine();

    bool init(HWND hwnd);
    bool reinit(HWND hwnd);
    void render();
    void cleanup();

    void resize(int width, int height);
    void reparent(HWND new_parent);
    
    void load_video(const std::string& path, MpvRuntimeOptions options);
    bool load_shader(const std::string& path);
    void pause_video();
    void resume_video();
    void set_mouse(float x, float y, bool clicking);

private:
    HWND m_hwnd = nullptr;
    HWND m_render_hwnd = nullptr;
    int m_width = 0;
    int m_height = 0;

    std::unique_ptr<VideoPlayer> m_video_player;
    MpvRuntimeOptions m_video_options{};
    std::unique_ptr<ShaderRenderer> m_shader_renderer;
    bool m_shader_mode = false;
};

} // namespace wp
