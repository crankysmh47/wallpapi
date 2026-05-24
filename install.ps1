$ErrorActionPreference = "Stop"

function Add-ToUserPath([string]$Dir) {
    $Dir = $Dir.TrimEnd('\')
    $envKey = "HKCU:\Environment"
    $current = [Environment]::GetEnvironmentVariable("Path", "User")
    if (-not $current) { $current = "" }
    $parts = $current -split ";" | Where-Object { $_ -and $_.Trim() -ne "" }
    $parts = $parts | Where-Object { $_.TrimEnd('\') -ne $Dir }
    $new = @($Dir) + $parts
    $value = ($new -join ";").TrimEnd(";")
    [Environment]::SetEnvironmentVariable("Path", $value, "User")
    Set-ItemProperty -Path $envKey -Name "Path" -Value $value
}

function Broadcast-EnvironmentChange {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class EnvBroadcast {
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessageTimeout(
        IntPtr hWnd, uint Msg, UIntPtr wParam, string lParam,
        uint fuFlags, uint uTimeout, out UIntPtr lpdwResult);
}
"@
    [UIntPtr]$result = [UIntPtr]::Zero
    [void][EnvBroadcast]::SendMessageTimeout(
        [IntPtr]0xffff, 0x001A, [UIntPtr]::Zero, "Environment", 2, 5000, [ref]$result)
}

function Install-CliShim([string]$InstallRoot) {
    $windowsApps = Join-Path $env:LOCALAPPDATA "Microsoft\WindowsApps"
    if (-not (Test-Path $windowsApps)) {
        New-Item -ItemType Directory -Force -Path $windowsApps | Out-Null
    }

    $shim = Join-Path $windowsApps "wp-cli.cmd"
    $target = Join-Path $InstallRoot "wp-cli.exe"
    @"
@echo off
"$target" %*
"@ | Set-Content -Path $shim -Encoding ASCII -Force
}

$BuildDir = Join-Path (Get-Location) "build\Release"
if (-not (Test-Path (Join-Path $BuildDir "wp-engine.exe"))) {
    Write-Host "Build output not found. Running build.ps1..."
    .\build.ps1
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$InstallRoot = Join-Path $env:LOCALAPPDATA "Programs\Wallpapi"
Write-Host "Installing to $InstallRoot"

New-Item -ItemType Directory -Force -Path $InstallRoot | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $InstallRoot "wallpapers") | Out-Null

Copy-Item -Path (Join-Path $BuildDir "wp-engine.exe") -Destination $InstallRoot -Force
Copy-Item -Path (Join-Path $BuildDir "wp-cli.exe") -Destination $InstallRoot -Force
Copy-Item -Path (Join-Path $BuildDir "wp-ui.exe") -Destination $InstallRoot -Force
Copy-Item -Path "config.toml" -Destination $InstallRoot -Force

if (Test-Path "wallpapers") {
    Copy-Item -Path "wallpapers\*" -Destination (Join-Path $InstallRoot "wallpapers") -Recurse -Force -ErrorAction SilentlyContinue
}

$Engine = Join-Path $InstallRoot "wp-engine.exe"
$StartupFolder = [Environment]::GetFolderPath("Startup")
$ShortcutPath = Join-Path $StartupFolder "Wallpapi.lnk"

$WshShell = New-Object -ComObject WScript.Shell
$Shortcut = $WshShell.CreateShortcut($ShortcutPath)
$Shortcut.TargetPath = $Engine
$Shortcut.WorkingDirectory = $InstallRoot
$Shortcut.Description = "Wallpapi Video Wallpaper Engine"
$Shortcut.Save()

Add-ToUserPath $InstallRoot
Install-CliShim $InstallRoot
Broadcast-EnvironmentChange

# Refresh PATH in this shell so install.ps1 can verify immediately.
$env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine") + ";" +
            [Environment]::GetEnvironmentVariable("Path", "User")

Write-Host ""
Write-Host "Verifying wp-cli..."
$null = Get-Command wp-cli -ErrorAction Stop
Write-Host "  Found: $((Get-Command wp-cli).Source)"
try {
    & wp-cli status
} catch {
    Write-Host "  (engine not running - start wp-engine, then: wp-cli status)"
}

Write-Host ""
Write-Host "------------------------------------------------"
Write-Host "SUCCESS!"
Write-Host "- Installed to: $InstallRoot"
Write-Host "- User PATH updated (prepended)"
Write-Host "- Shim: $env:LOCALAPPDATA\Microsoft\WindowsApps\wp-cli.cmd"
Write-Host "- Startup shortcut: $ShortcutPath"
Write-Host ""
Write-Host "If an open terminal still cannot find wp-cli, close and reopen it"
Write-Host "(or restart Cursor). New terminals will work immediately."
Write-Host "------------------------------------------------"
