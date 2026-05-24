# Wallpapi

Minimal live wallpaper engine for Windows — video on the desktop using Windows Media Foundation (no libmpv).

## What it does

- Plays video or image wallpapers on the desktop (WorkerW layer)
- Pauses automatically during fullscreen apps and on battery (configurable)
- Recovers after sleep / wake
- Controlled from any terminal via `wp-cli`

## Quick start

```powershell
.\build.ps1
.\run-engine.ps1
```

In another terminal:

```powershell
.\build\Release\wp-cli.exe status
.\build\Release\wp-cli.exe set "wallpapers\your-video.mp4"
```

## Install (PATH + startup)

```powershell
.\install.ps1
```

Then open a **new** terminal:

```powershell
wp-cli status
wp-cli list
wp-cli set "C:\path\to\video.mp4"
```

Or register from the build folder without copying:

```powershell
.\register-startup.ps1
```

## Config (`config.toml`)

```toml
video = "wallpapers/example.mp4"
muted = true
pause_on_battery = true
pause_on_fullscreen = true
```

Drop media into `wallpapers/` — if `video` is empty, the first file found is used.

## CLI commands

| Command | Description |
|---------|-------------|
| `wp-cli set <path>` | Set video or image wallpaper |
| `wp-cli status` | Current path, paused, battery, fullscreen state |
| `wp-cli list` | Files in `wallpapers/` |
| `wp-cli pause` / `resume` | Manual pause/resume |
| `wp-cli stop` | Shut down engine |

## Legacy code

Shaders, Lua scripting, tests, and the old spec live in `_legacy/` (gitignored). The full previous version remains in git history.
