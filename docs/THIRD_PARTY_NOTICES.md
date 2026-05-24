# Third-Party Notices

Wallpapi includes or depends on the following third-party components.

## Bundled at build time (source dependencies)

### toml11

- **Purpose:** Parse `config.toml`
- **License:** MIT License
- **Source:** https://github.com/ToruNiina/toml11 (v3.7.1)
- **Fetched by:** CMake FetchContent during build

### spdlog

- **Purpose:** Logging
- **License:** MIT License
- **Source:** https://github.com/gabime/spdlog (v1.12.0)
- **Fetched by:** CMake FetchContent during build

## System components (not redistributed)

### Windows Media Foundation

- **Purpose:** Video and image playback on the desktop
- **License:** Part of Windows; subject to Microsoft’s Windows terms
- **Used via:** `mfplat`, `mfreadwrite`, and related Windows APIs

### Windows Desktop APIs

- **Purpose:** Desktop wallpaper layer, fullscreen detection, power status, DWM effects in `wp-ui`
- **License:** Part of Windows

## Legacy headers in `deps/include/mpv`

The repository may contain **libmpv** header files under `deps/` for historical or optional tooling. The **minimal Wallpapi engine does not require libmpv** at runtime. If you build components that link libmpv, comply with libmpv’s license (typically LGPL v2.1+).

## Sample wallpapers

Files under `wallpapers/` in the repository (if present) are sample media for demonstration. Verify licensing before redistribution of those specific files.
