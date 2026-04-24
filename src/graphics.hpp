#pragma once
#include <windows.h>
#include <memory>
#include <string>

// mpv headers will be included in the implementation
struct mpv_handle;

namespace wp {

class VideoPlayer {
public:
    VideoPlayer(HWND hwnd);
    ~VideoPlayer();

    bool load(const std::string& path);
    void play();
    void pause();

private:
    mpv_handle* m_mpv = nullptr;
};

class GraphicsEngine {
public:
    GraphicsEngine();
    ~GraphicsEngine();

    bool init(HWND hwnd);
    void render();
    void cleanup();

    void resize(int width, int height);
    
    void load_video(const std::string& path);

private:
    HWND m_hwnd = nullptr;
    int m_width = 0;
    int m_height = 0;

    std::unique_ptr<VideoPlayer> m_video_player;
};

} // namespace wp
