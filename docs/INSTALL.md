# Installing Wallpapi on Windows

Wallpapi runs on **Windows 10 or Windows 11** (64-bit).

## Option A — Download a release (recommended)

1. Open [GitHub Releases](https://github.com/crankysmh47/wallpapi/releases) and download the latest `Wallpapi-*-win64.zip`.
2. Extract the zip to a folder (e.g. `C:\Wallpapi`).
3. Open **PowerShell** in that folder.
4. Run:

   ```powershell
   Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
   .\install.ps1
   ```

5. Accept the license prompt when asked.
6. Close and reopen your terminal, then verify:

   ```powershell
   wp-cli status
   ```

The installer registers startup, adds Wallpapi to your user PATH, and creates shortcuts.

## Option B — Clone from GitHub (build from source)

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/) with **Desktop development with C++** and [CMake](https://cmake.org/download/).
2. Clone and install:

   ```powershell
   git clone https://github.com/crankysmh47/wallpapi.git
   cd wallpapi
   .\install.ps1
   ```

   `install.ps1` builds the project automatically if binaries are missing.

## After installation

| Tool | Description |
|------|-------------|
| `wp-engine` | Wallpaper engine (starts at login if startup shortcut is enabled) |
| `wp-cli` | Terminal commands (`wp-cli list`, `wp-cli set`, …) |
| `wp-ui` | Graphical control panel |

**Default install location:** `%LOCALAPPDATA%\Programs\Wallpapi`

**Sample wallpapers:** `%LOCALAPPDATA%\Programs\Wallpapi\wallpapers`

**Config file:** `%LOCALAPPDATA%\Programs\Wallpapi\config.toml`

### Quick commands

```powershell
wp-cli list
wp-cli set "wallpapers\your-video.mp4"
wp-ui
```

## Uninstall

From the install folder or a clone of the repo:

```powershell
.\uninstall.ps1
```

Or run `%LOCALAPPDATA%\Programs\Wallpapi\uninstall.ps1` if you kept a copy there.

## Troubleshooting

| Problem | What to try |
|---------|-------------|
| `wp-cli` not found | Open a **new** terminal after install |
| Engine not running | Start `wp-engine` from Start menu or `run-engine.ps1` |
| Black desktop / no wallpaper | Ensure a valid file is in `config.toml` or `wallpapers\` |
| Build fails | Install VS 2022 C++ workload; run `cmake --version` |

## License

Installation requires acceptance of the [End User License Agreement](EULA.md). The software is also provided under the [MIT License](../LICENSE).
