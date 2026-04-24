# Wallpapi

A minimal, high-performance desktop video wallpaper engine for Windows.

## Features
- **Zero Overhead**: Renders directly to the desktop WorkerW layer using libmpv.
- **Native Performance**: Uses hardware acceleration (DirectX/Vulkan) via mpv's gpu-next VO.
- **Auto-Pause**: Automatically pauses video when fullscreen apps are detected or on battery.
- **IPC Support**: Change wallpapers via command line or scripts.

## Build
Run `.\build.ps1` (requires CMake and MSVC).

## How to Run
1.  **Clone the Repo**: Ensure you have `libmpv-2.dll` in your path or the build folder.
2.  **Build**: Run `.\build.ps1` in PowerShell.
3.  **Launch**: Run `.\build\Release\wp-engine.exe`.
    - If your `config.toml` is empty or invalid, Wallpapi will automatically scan the `wallpapers/` folder and play the first MP4 it finds.

## Auto-Start (Windows)
To make Wallpapi start automatically when you log in or wake your PC:
1.  Press `Win + R`, type `shell:startup`, and hit Enter.
2.  Right-click in the folder and select **New > Shortcut**.
3.  Browse to your `wp-engine.exe` location.
4.  Wallpapi will now launch hidden in the background every time you start Windows.

## Controls
- **CLI**: Use `wp-cli.exe set-video "path/to/video.mp4"` to change wallpaper.
- **Config**: Edit `config.toml` to toggle auto-pause on battery or fullscreen.
