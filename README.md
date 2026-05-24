# Wallpapi

[![CI (Windows)](https://github.com/crankysmh47/wallpapi/actions/workflows/ci-windows.yml/badge.svg)](https://github.com/crankysmh47/wallpapi/actions/workflows/ci-windows.yml)

Minimal live wallpaper engine for Windows — video and images on your desktop using **Windows Media Foundation** (no libmpv required at runtime).

## Download and install (end users)

**Requirements:** Windows 10 or 11 (64-bit)

### From a release (easiest)

1. Download the latest **`Wallpapi-*-win64.zip`** from [Releases](https://github.com/crankysmh47/wallpapi/releases).
2. Extract the folder anywhere (e.g. `C:\Wallpapi`).
3. Open **PowerShell** in that folder and run:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
.\install.ps1
```

4. Accept the license agreement when prompted.
5. Open a **new** terminal and run:

```powershell
wp-cli status
wp-ui
```

The engine starts at login via a startup shortcut. Use the **Wallpapi** desktop shortcut to open the control panel.

### From source (developers)

```powershell
git clone https://github.com/crankysmh47/wallpapi.git
cd wallpapi
.\install.ps1
```

You need **Visual Studio 2022** (Desktop C++) and **CMake 3.20+**. The installer builds automatically if binaries are missing.

Full instructions: [docs/INSTALL.md](docs/INSTALL.md)

## What you get

| Component | Purpose |
|-----------|---------|
| `wp-engine` | Plays wallpapers on the desktop layer; pauses on battery/fullscreen |
| `wp-cli` | Terminal control — list, set, config, toggles |
| `wp-ui` | Minimal graphical panel for everyday use |

**Install location:** `%LOCALAPPDATA%\Programs\Wallpapi`

## Quick start (after install)

```powershell
wp-cli list
wp-cli set "wallpapers\your-video.mp4"
wp-cli status
wp-ui
```

## Configuration

Edit `%LOCALAPPDATA%\Programs\Wallpapi\config.toml` or use `wp-cli` / `wp-ui`:

```toml
video = "wallpapers/example.mp4"
muted = true
pause_on_battery = true
pause_on_fullscreen = true
```

Drop media into the `wallpapers` folder. If `video` is empty, the first supported file is used automatically.

## CLI reference

| Command | Description |
|---------|-------------|
| `wp-cli set <path>` | Set wallpaper |
| `wp-cli list` | List `wallpapers/` |
| `wp-cli next` / `random` | Cycle or random wallpaper |
| `wp-cli add <path>` | Copy into `wallpapers/` and apply |
| `wp-cli open` | Open wallpapers folder |
| `wp-cli status` | Playback and system state |
| `wp-cli config get` | Show settings |
| `wp-cli config set <key> <true\|false>` | `pause_on_battery`, `pause_on_fullscreen`, `muted` |
| `wp-cli toggle pause_on_battery` | Toggle setting |
| `wp-cli pause` / `resume` | Manual playback control |
| `wp-cli stop` | Shut down engine |

## Build, package, and test

```powershell
.\build.ps1              # Compile Release binaries
.\test-install.ps1       # Smoke test (CI runs this)
.\package.ps1            # Create dist\Wallpapi-<version>-win64.zip
```

## Uninstall

```powershell
.\uninstall.ps1
```

Or run `%LOCALAPPDATA%\Programs\Wallpapi\uninstall.ps1`.

## Legal

| Document | Description |
|----------|-------------|
| [LICENSE](LICENSE) | MIT License (open source) |
| [docs/EULA.md](docs/EULA.md) | End User License Agreement (shown during install) |
| [docs/THIRD_PARTY_NOTICES.md](docs/THIRD_PARTY_NOTICES.md) | Dependency attributions |

By installing, you accept the EULA. The software is provided **as is** without warranty.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Report security issues per [SECURITY.md](SECURITY.md).

## Changelog

See [CHANGELOG.md](CHANGELOG.md).
