# The Wallpaper Engine Killer

A high-performance, lightweight, and zero-dependency (almost) wallpaper engine for Windows. Built with C++20, `libmpv`, `sokol_gfx`, and Lua.

## Core Features
- **Win32 Desktop Hijack**: Renders directly behind desktop icons using the `WorkerW` technique.
- **Hybrid Rendering**: Combines hardware-accelerated video playback (`libmpv`) with custom GLSL shaders (`sokol_gfx`).
- **Zero Bloat**: Native C++ implementation with minimal overhead (~300 KB binary).
- **Hot-Reloading**: Real-time updates for TOML configuration and shaders.
- **Lua Scripting**: Event-driven automation (e.g., pause on battery, change on time).
- **IPC Control**: Command-line interface for remote control and integration.

## Architecture
The engine is split into two main components:
1.  **`wp-engine`**: The core rendering service that lives behind your desktop.
2.  **`wp-cli`**: A lightweight utility to talk to the engine via Named Pipes.

## Technical Stack
- **Build System**: CMake 3.20+
- **Configuration**: [toml11](https://github.com/ToruNiina/toml11)
- **Scripting**: [Lua 5.4](https://www.lua.org/)
- **Graphics Backend**: [sokol_gfx](https://github.com/floooh/sokol) (D3D11/GL)
- **Video Backend**: [libmpv](https://mpv.io/manual/master/#library)

## Installation & Setup
(Coming Soon)

## Usage
### Running the Engine
```bash
wp-engine.exe
```

### Controlling via CLI
```bash
wp-cli set-video "wallpapers/sunset.mp4"
wp-cli set-shader "shaders/wave.glsl"
wp-cli pause
```

## Development
To build the project:
1. Ensure CMake and a C++20 compiler (MSVC 2019+) are installed.
2. Place `libmpv` binaries in the `deps/` folder.
3. Run:
   ```bash
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```
