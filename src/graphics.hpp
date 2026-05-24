#pragma once
#include <windows.h>
#include <memory>
#include <string>
#include "video_options.hpp"

namespace wp {

class VideoPlayer {
public:
    VideoPlayer(HWND hwnd, VideoOptions options);
    ~VideoPlayer();

    bool load(const std::string& path);
    void play();
    void pause();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

class GraphicsEngine {
public:
    GraphicsEngine();
    ~GraphicsEngine();

    bool init(HWND hwnd);
    bool reinit(HWND hwnd);
    void cleanup();

    void resize(int width, int height);
    void reparent(HWND new_parent);

    void load_video(const std::string& path, VideoOptions options);
    void pause_video();
    void resume_video();
    void release_video();

private:
    HWND m_hwnd = nullptr;
    HWND m_render_hwnd = nullptr;
    HWND m_video_hwnd = nullptr;
    int m_width = 0;
    int m_height = 0;

    std::unique_ptr<VideoPlayer> m_video_player;
    VideoOptions m_video_options{};
};

bool media_foundation_startup();
void media_foundation_shutdown();

} // namespace wp
