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
To make Wallpapi start automatically when you log in:
1.  Open PowerShell as Administrator in the project folder.
2.  Run `.\register-startup.ps1`.
3.  That's it! The script creates a shortcut in your `shell:startup` folder and correctly sets the "Start In" directory so it can find its library files.

## Controls
- **CLI**: Use `wp-cli.exe set-video "path/to/video.mp4"` to change wallpaper.
- **Config**: Edit `config.toml` to toggle auto-pause on battery or fullscreen.
