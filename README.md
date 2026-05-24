# Wallpapi

Minimal live wallpaper engine for Windows — video on the desktop using Windows Media Foundation (no libmpv).

## What it does

- Plays video or image wallpapers on the desktop (WorkerW layer)
- Pauses automatically during fullscreen apps and on battery (configurable)
- Recovers after sleep / wake
- Controlled from any terminal via `wp-cli`
- Graphical control panel via `wp-ui` for non-terminal users

## Quick start

```powershell
.\build.ps1
.\run-engine.ps1
```

In another terminal:

```powershell
.\build\Release\wp-cli.exe status
.\build\Release\wp-cli.exe list
.\build\Release\wp-cli.exe set "wallpapers\your-video.mp4"
```

Or launch the control panel:

```powershell
.\build\Release\wp-ui.exe
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
wp-ui
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
| `wp-cli list` | Files in `wallpapers/` |
| `wp-cli next` | Cycle to next wallpaper |
| `wp-cli random` | Pick a random wallpaper |
| `wp-cli add <path>` | Copy file into `wallpapers/` and apply |
| `wp-cli open` | Open `wallpapers/` in Explorer |
| `wp-cli status` | Current path, paused, battery, fullscreen state |
| `wp-cli config get` | Show config values |
| `wp-cli config set <key> <true\|false>` | Set `pause_on_battery`, `pause_on_fullscreen`, or `muted` |
| `wp-cli toggle pause_on_battery` | Toggle battery pause |
| `wp-cli toggle pause_on_fullscreen` | Toggle fullscreen pause |
| `wp-cli toggle muted` | Toggle mute |
| `wp-cli pause` / `resume` | Manual pause/resume |
| `wp-cli stop` | Shut down engine |

## Control panel (`wp-ui`)

Lightweight popup for everyday use:

- Toggle pause-on-battery, pause-on-fullscreen, and mute
- Pause/resume and skip to next wallpaper
- Browse wallpapers (double-click to apply)
- Add wallpapers via file picker or open the wallpapers folder

Requires `wp-engine` to be running.

## Legacy code

Shaders, Lua scripting, tests, and the old spec live in `_legacy/` (gitignored). The full previous version remains in git history.
