# Wallpapi — Project Specification & Roadmap

> Last updated: May 2026  
> Status key: ✅ Done · 🔧 Partially done / buggy · ❌ Not started

---

## 1. Core Engine

### 1.1 Desktop Integration (WorkerW Layer)
- ✅ Enumerate WorkerW via `SendMessage(Progman, 0x052C)` trick
- ✅ Multi-method WorkerW discovery (sibling search, DefView search, fullscreen size fallback, Progman child fallback)
- ✅ Render child window embedded in WorkerW layer (`WS_CHILD | WS_VISIBLE`)
- ✅ Desktop icon z-ordering (render window placed behind `SHELLDLL_DefView`)
- ✅ Single-instance enforcement via named mutex
- ✅ High DPI awareness (`SetProcessDPIAware`)
- ✅ Working directory auto-set to executable folder (resources always findable)
- 🔧 **Multi-monitor support** — engine only embeds into the primary monitor's WorkerW; secondary monitors show static wallpaper. Needs per-monitor GraphicsEngine instances.
- ❌ Per-monitor wallpaper assignment (different video/image per screen)

### 1.2 Video Playback
- ✅ Hardware-accelerated video via libmpv (`vo=gpu-next`, `hwdec=auto`)
- ✅ Infinite loop (`loop-file=inf`)
- ✅ Input bindings disabled (no mpv UI interference)
- ✅ Pause / resume API
- ✅ Hot-swap video at runtime via IPC
- ✅ **`muted` config field applied** — mpv is now started with `mute=yes/no` based on config, and reloads player on change
- ✅ **`fps_limit` applied** — main render loop throttles to the configured FPS limit when rendering shaders
- ✅ Image wallpaper support (static JPG/PNG/WebP) — wallpaper fallback scan now includes common image formats (loaded via mpv)
- ❌ GIF support
- ❌ Audio volume control (set via IPC / config)
- ❌ Playback speed control

### 1.3 Shader Renderer (D3D11)
- ✅ D3D11 device + swapchain with `FLIP_DISCARD` swap effect
- ✅ Fullscreen triangle vertex shader (no vertex buffer needed)
- ✅ ShaderToy-compatible uniform block (`iResolution`, `iTime`, `iTimeDelta`, `iFrame`, `iMouse`, `iPrevMouse`)
- ✅ GLSL → HLSL compatibility shim (`shaders/compat.hlsl`)
- ✅ Ping-pong simulation buffers (`iChannel0`) for stateful shaders
- ✅ Mouse position tracking passed to shader uniforms
- ✅ Runtime shader hot-reload (swap via IPC or config change)
- ✅ Resize handling (ping-pong buffers recreated on resize)
- 🔧 **Display pass hardcoded for fluid_ns look** — `compile_display_shader()` decodes `RG=velocity, A=ink`, so any other stateful shader (e.g. a custom reaction-diffusion) would render incorrectly. Display pass should be user-configurable or generic (just blit `iChannel0`).
- 🔧 **Fluid animation broken / not smooth** — `fluid.glsl` is single-pass curl noise (no persistent state) so mouse drag is approximated and feels laggy. `fluid_ns.glsl` uses the proper stateful path but the advection timestep (`vel * 0.001`) is very conservative and the pressure solver runs only one Jacobi iteration per frame, making it underdamped and jittery.
- ❌ Fix `fluid_ns.glsl` to feel polished: increase advection step, add 4–8 pressure Jacobi iterations, smooth ink injection radius, add velocity viscosity, ensure cursor velocity is properly derived from `iPrevMouse` delta
- ❌ Generic display pass (blit-only) so non-fluid stateful shaders display correctly
- ❌ `iChannel1` / texture input support (load an image as a shader texture)
- ❌ Shader error overlay (show compilation errors on-screen instead of silently failing)

---

## 2. Configuration System

- ✅ TOML config file (`config.toml`) with hot-reload via `ReadDirectoryChangesW`
- ✅ Fields: `video`, `shader`, `muted`, `fps_limit`
- ✅ Config-watcher callback triggers live wallpaper swap
- ✅ **`pause_on_battery` and `pause_on_fullscreen` are parsed and respected at runtime** (battery auto-pause + fullscreen pause are now config-controlled)
- ✅ **Last wallpaper persistence** — engine writes back the last-used wallpaper/shader to `config.toml` on IPC `set-video` / `set-shader`
- ✅ Add `pause_on_battery` and `pause_on_fullscreen` fields to `Config` struct and wire them to `Monitor`/`g_system_state`
- ✅ Persist last-used wallpaper path to config on `set-video` / `set-shader` IPC commands
- ❌ Config schema validation with human-readable errors
- ✅ Support relative paths in config (resolve relative to config file location)
- ❌ `volume` config field (0–100)
- ❌ `playback_speed` config field

---

## 3. IPC & CLI

### 3.1 IPC Server (Named Pipe)
- ✅ Named pipe server (`\\.\pipe\wp_engine_pipe`)
- ✅ Commands: `set-video <path>`, `set-image <path>`, `set-shader <path>`, `pause`, `resume`, `get-status`, `list-wallpapers`, `stop`
- ✅ Pipe supports multiple instances (`PIPE_UNLIMITED_INSTANCES`) and larger buffers
- ✅ IPC responses — engine returns `OK ...` or `ERR ...` and CLI prints it / exits non-zero on error
- ✅ `get-status` command (returns current wallpaper path, paused state, mode)
- ✅ `list-wallpapers` command (scans wallpapers/ folder and returns paths)
- ✅ `set-image <path>` command alias
- ✅ `stop` / `quit` command to gracefully shut down the engine

### 3.2 wp-cli Tool
- ✅ Connects to named pipe and forwards arguments as a command string
- ✅ Basic error message if engine is not running
- 🔧 **wp-cli not added to user PATH** — users must call it with a full path or from the build directory
- ❌ Add `wp-cli` to `%USERPROFILE%\AppData\Local\Microsoft\WindowsApps` or `%LOCALAPPDATA%\Programs\Wallpapi` and register in user PATH via installer/register script
- ❌ `wp list` command — scans wallpapers/ and shaders/ folders, prints available files
- ❌ `wp status` command — prints current wallpaper, mode (video/shader/image), paused state
- ❌ `wp pause` / `wp resume` / `wp stop` as first-class subcommands with help text
- ❌ `wp set <path>` — smart command that auto-detects video vs image vs shader by extension
- ❌ Shell completion script (PowerShell tab-complete for `wp` commands)

---

## 4. System Event Handling

### 4.1 Power Management
- ✅ `WM_POWERBROADCAST` handler for suspend/resume
- ✅ Battery detection and auto-pause when on battery
- ✅ Resume: re-verifies WorkerW host and re-initializes graphics if host changed
- ✅ Resume: re-parents render window if WorkerW address changed
- 🔧 **Wake-to-wallpaper-not-loading bug** — on some machines Windows resets the WorkerW hierarchy on resume; the current re-init path (`g_graphics->init()`) creates a new child window but doesn't destroy the old one, leading to orphaned windows. Full teardown + reinit sequence needed.
- 🔧 **Multi-monitor sleep/wake bug** — on machines with multiple displays, the WorkerW may be recreated for each monitor independently on wake; current code only tracks a single `g_wallpaper_host` pointer.
- ✅ Full graphics teardown + clean reinit on wake (destroy old render window, create new one, reload wallpaper)
- ❌ Per-monitor wake recovery

### 4.2 Fullscreen / Gaming Detection
- ✅ `SetWinEventHook` on `EVENT_SYSTEM_FOREGROUND` to detect fullscreen apps
- ✅ Auto-pause (`g_system_state.is_gaming = true`) when fullscreen non-desktop window detected
- ✅ Auto-resume when fullscreen app loses focus
- 🔧 **`pause_on_fullscreen` config flag exists in TOML but is not wired** — fullscreen pause always active regardless of config
- ✅ Respect `pause_on_fullscreen = false` config flag

---

## 5. Startup & Installation (Windows)

- ✅ `register-startup.ps1` — creates shortcut in `shell:startup` folder
- ✅ `build.ps1` — CMake build script with auto DLL copy
- ✅ `setup_deps.ps1` — auto-downloads latest libmpv dev build from GitHub releases
- ✅ Desktop "Change Wallpaper" shortcut created by register script
- ✅ Exponential backoff retry loop (up to 20 attempts) for shell readiness on boot
- 🔧 **Cold startup is ~10 seconds** — engine is launched from `shell:startup` which fires after the desktop is fully loaded. An early-launch trigger is needed.
- ❌ **Early-launch startup path** — register engine in `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` with a slight delay (`Start-Sleep 3`) OR use a Task Scheduler task triggered on `SessionUnlock` / `AtLogon` with a delay to start before desktop icons are drawn
- ❌ **Task Scheduler option** — create a scheduled task triggered at logon (no UAC, runs as current user) for more reliable startup than shell:startup
- ❌ `unregister-startup.ps1` — removes startup shortcut and/or scheduled task
- ❌ Proper installer (NSIS or Inno Setup) that:
  - Copies binaries + DLLs + shaders + wallpapers to `%LOCALAPPDATA%\Programs\Wallpapi`
  - Adds `wp-cli.exe` to user PATH
  - Registers startup shortcut
  - Creates Start Menu shortcut for the GUI
  - Provides uninstaller
- ❌ Pre-built release ZIP/installer on GitHub Releases (CI-built via GitHub Actions)
- ❌ GitHub Actions workflow to auto-build and publish releases on tag push

---

## 6. GUI (Windows)

- 🔧 **Only interaction is CLI or editing config.toml manually** — no graphical interface
- ❌ **System tray icon** — right-click menu with: Change Wallpaper, Pause/Resume, Open Settings, Quit
- ❌ **Wallpaper picker dialog** (the existing `select_wallpaper.ps1` is a PowerShell stopgap) — replace with a native dialog or Electron/Tauri/WinUI3 app
- ❌ Thumbnail preview grid of available wallpapers in wallpapers/ folder
- ❌ Drag-and-drop a video/image onto the tray icon to set it as wallpaper
- ❌ Settings panel: fps limit, mute toggle, pause-on-battery toggle, pause-on-fullscreen toggle
- ❌ "Now playing" tooltip on tray icon showing current wallpaper filename
- ❌ Minimize-to-tray on close (if ever a main window is added)

---

## 7. Linux Support

- ❌ **Linux compatibility** — entire codebase is Windows-only (Win32 API, HWND, D3D11, named pipes)
- ❌ Abstract platform layer: `IPlatform` interface with `Win32Platform` and `LinuxPlatform` implementations
- ❌ Linux: embed wallpaper via `xwinwrap` + mpv (X11) or `swaybg` replacement approach (Wayland/wlroots)
- ❌ Linux: IPC via Unix domain socket instead of named pipe
- ❌ Linux: Startup via `~/.config/autostart` `.desktop` file or systemd user service
- ❌ Linux: OpenGL or Vulkan shader renderer (D3D11 not available)
- ❌ Linux build via CMake with `pkg-config` for mpv
- ❌ Linux DEB / AUR package

---

## 8. Code Quality & Architecture

### 8.1 Existing Bugs / Silent Failures
- ✅ Merge conflicts in `main.cpp` and `register-startup.ps1` — **resolved in this session**
- 🔧 **`muted` config field never applied to mpv** — add `mpv_set_option_string(m_mpv, "mute", muted ? "yes" : "no")` in `VideoPlayer` constructor or on load
- 🔧 **`fps_limit` never applied** — add `mpv_set_option(m_mpv, "fps", MPV_FORMAT_INT64, &fps)` or throttle the render loop
- 🔧 **IPC `pause`/`resume` commands only set the state flag** — they don't actually call `g_graphics->pause_video()` / `g_graphics->resume_video()`
- 🔧 **Auto-fallback only scans for `.mp4`** — should also scan `.mkv`, `.avi`, `.webm`, `.jpg`, `.png`, `.gif`
- 🔧 **Shader path in config not resolved relative to engine** — absolute paths required; relative paths from config.toml directory should work
- ❌ Fix IPC `pause`/`resume` to call graphics engine pause/resume methods
- ❌ Fix muted / fps_limit wiring

### 8.2 LuaEngine
- ✅ `LuaEngine` class exists and is initialized
- ❌ **LuaEngine is a complete stub** — `init()` does nothing; Lua is compiled in as a dependency but unused. Either implement scripting support or remove the dead code and Lua dependency.
- ❌ Lua scripting API: expose `set_video()`, `set_shader()`, `get_time()`, `on_hour()`, schedule-based wallpaper rotation

### 8.3 Multi-Instance / Pipe Robustness
- ❌ IPC pipe: set `nMaxInstances` to `PIPE_UNLIMITED_INSTANCES`
- ❌ IPC: send acknowledgement response back to client (success/error string)
- ❌ IPC: handle commands longer than 512 bytes (current buffer limit)

---

## 9. Shaders (Shelved for now — revisit later)

> Shaders are deprioritized. The section below is kept for future reference.

- ✅ `fluid.glsl` — single-pass curl noise (mouse reactive, no state)
- ✅ `fluid_ns.glsl` — stateful Navier-Stokes simulation (ping-pong buffers)
- ✅ `nebula.glsl` — space nebula animated shader
- ✅ `plasma.glsl` — classic plasma / color wave shader
- ✅ `test.glsl` — UV debug / gradient test shader
- ✅ `wings.glsl` — wings/butterfly motion shader
- ✅ `shaders/compat.hlsl` — GLSL-to-HLSL compatibility macros
- 🔧 All shaders are untested at proper 4K / 1440p resolutions
- ❌ **Fix `fluid_ns.glsl` to feel smooth and polished:**
  - Increase advection timestep (`vel * 0.003` or higher)
  - Run 4–8 Jacobi pressure iterations per frame
  - Smooth ink injection with a Gaussian kernel instead of hard `smoothstep`
  - Add velocity diffusion/viscosity term
  - Derive mouse velocity properly from `iPrevMouse` delta each frame
  - Clamp velocity magnitude to prevent blow-up
- ❌ Generic blit display pass so non-fluid stateful shaders render correctly
- ❌ Curated shader library: find / author 2–3 high-quality animated wallpaper shaders

---

## 10. Release & Distribution

- ❌ **Consumers currently must compile C++ from source** — this is a hard barrier
- ❌ Pre-built Windows binary release (GitHub Actions → Release ZIP)
  - `wp-engine.exe`
  - `wp-cli.exe`
  - `libmpv-2.dll`
  - `shaders/` folder
  - `wallpapers/` placeholder folder
  - `config.toml` default
  - `install.ps1` (one-click setup)
- ✅ `install.ps1` script that:
  - Copies files to `%LOCALAPPDATA%\Programs\Wallpapi`
  - Adds `wp-cli` to user PATH via registry
  - Registers startup task
  - Opens the wallpapers folder so user can drop in their videos
- ✅ GitHub Actions CI: builds on Windows and runs tests (adds `setup_deps.ps1`, builds `wp-engine` and `wp-tests`)
- ❌ Auto-update check on startup (query GitHub releases API, notify via tray)

---

## 11. Tiny / Quality-of-Life Improvements

- ❌ Log rotation — `latest.log` grows unbounded; cap at 1 MB and roll to `latest.log.1`
- ❌ Configurable log level via `config.toml` (`log_level = "info"`)
- ❌ `wp-cli set <path>` — smart set that auto-detects type by extension (video/image/shader)
- ❌ `wp-cli list` — print all files in `wallpapers/` and `shaders/` with index numbers
- ❌ `wp-cli next` / `wp-cli prev` — cycle through wallpapers in folder alphabetically
- ❌ Shuffle mode: randomly pick from wallpapers/ folder on each startup
- ❌ Scheduled rotation: change wallpaper every N minutes (Lua scripting can power this)
- ❌ Tray icon right-click → "Open wallpapers folder" shortcut
- ❌ Startup splash / first-run wizard: asks user to pick a wallpaper on first launch
- ❌ `--no-startup` CLI flag to run once and exit (for testing without autostart)
- ❌ Verbose mode flag (`--verbose`) that enables debug logging at runtime
- ❌ Windows dark mode tray icon (high-contrast white icon variant)
- ❌ Wallpaper transition effect on change (crossfade / dissolve, 500 ms)
- ❌ Pause wallpaper when display is off / screen locked (register `WTS_SESSION_LOCK` via `WTSRegisterSessionNotification`)
- ❌ Resume wallpaper only after user unlocks (not just on system wake)
- ❌ Battery threshold config: pause only below N% battery, not all battery (`pause_on_battery_below = 20`)
- ❌ Detect RDP / remote desktop session and auto-pause (no point rendering for a remote viewer)

---

## Priority Order (Suggested)

| # | Item | Effort |
|---|------|--------|
| 1 | Fix wake-to-blank-wallpaper (full teardown + reinit on resume) | Small |
| 2 | Fix IPC `pause`/`resume` to call graphics methods | Tiny |
| 3 | Persist last-used wallpaper to config on `set-video`/`set-shader` | Small |
| 4 | Add `pause_on_battery` / `pause_on_fullscreen` to Config struct | Small |
| 5 | Fix `muted` and `fps_limit` wiring to mpv | Small |
| 6 | Image wallpaper support (`.jpg`, `.png`, `.webp`) in auto-fallback + IPC | Small |
| 7 | Early-launch via Task Scheduler (cut startup time from ~10s to ~2s) | Small |
| 8 | Add `wp-cli` to PATH via register script | Small |
| 9 | `wp list`, `wp status`, `wp set <path>` commands | Medium |
| 10 | Fix `fluid_ns.glsl` to be smooth and cursor-reactive | Medium |
| 11 | System tray icon with right-click menu | Medium |
| 12 | Fix multi-monitor sleep/wake | Medium |
| 13 | Generic display blit pass for non-fluid stateful shaders | Small |
| 14 | GitHub Actions release pipeline + installer ZIP | Medium |
| 15 | Full GUI wallpaper picker (replace PowerShell dialog) | Large |
| 16 | Linux platform layer (X11 / Wayland) | Very Large |
