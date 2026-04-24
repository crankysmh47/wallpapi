# Wallpapi

A minimal, high-performance desktop video wallpaper engine for Windows.

## Features
- **Zero Overhead**: Renders directly to the desktop WorkerW layer using libmpv.
- **Native Performance**: Uses hardware acceleration (DirectX/Vulkan) via mpv's gpu-next VO.
- **Auto-Pause**: Automatically pauses video when fullscreen apps are detected or on battery.
- **IPC Support**: Change wallpapers via command line or scripts.

## Build
Run `.\build.ps1` (requires CMake and MSVC).

## Usage
1. Edit `config.toml` to set your video path.
2. Run `wp-engine.exe`.
3. Use `wp-cli.exe set-video "path/to/video.mp4"` to change videos on the fly.
