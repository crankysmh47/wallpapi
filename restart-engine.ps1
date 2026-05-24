$ErrorActionPreference = "Stop"
$Root = Join-Path $env:LOCALAPPDATA "Programs\Wallpapi"

Get-Process wp-engine -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep 1

if (-not (Test-Path (Join-Path $Root "wp-engine.exe"))) {
    Write-Error "Wallpapi not installed. Run .\install.ps1 from the project folder first."
    exit 1
}

@'
video = "wallpapers/goku.mp4"
muted = true
pause_on_battery = true
pause_on_fullscreen = true
'@ | Set-Content (Join-Path $Root "config.toml") -Encoding UTF8

Start-Process (Join-Path $Root "wp-engine.exe") -WorkingDirectory $Root
Start-Sleep 3

$env:Path = [Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [Environment]::GetEnvironmentVariable("Path","User")
wp-cli status
