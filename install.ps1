# Wallpapi installer — builds (if needed), installs to user Programs, updates PATH, registers startup.
param(
    [switch]$AcceptLicense,
    [switch]$SkipBuild,
    [switch]$SkipStartup,
    [switch]$SkipDesktopShortcut,
    [string]$InstallRoot = $(Join-Path $env:LOCALAPPDATA "Programs\Wallpapi")
)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
Set-Location $Root

function Show-Eula {
    param([switch]$Accepted)
    $eulaPath = Join-Path $Root "docs\EULA.md"
    if (-not (Test-Path $eulaPath)) {
        Write-Warning "EULA not found at $eulaPath; continuing without display."
        return $true
    }
    if ($Accepted) { return $true }

    Write-Host ""
    Write-Host "============================================================"
    Write-Host "  Wallpapi — End User License Agreement"
    Write-Host "============================================================"
    Write-Host ""
    Get-Content $eulaPath | Select-Object -First 45 | ForEach-Object { Write-Host $_ }
    Write-Host ""
    Write-Host "... (full text in docs\EULA.md)"
    Write-Host ""
    Write-Host "The Software is also licensed under the MIT License (see LICENSE)."
    Write-Host ""
    $answer = Read-Host "Do you accept these terms? [y/N]"
    return ($answer -match '^(y|yes)$')
}

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

function Install-CommandShim([string]$Name, [string]$InstallRoot) {
    $windowsApps = Join-Path $env:LOCALAPPDATA "Microsoft\WindowsApps"
    if (-not (Test-Path $windowsApps)) {
        New-Item -ItemType Directory -Force -Path $windowsApps | Out-Null
    }
    $shim = Join-Path $windowsApps "$Name.cmd"
    $target = Join-Path $InstallRoot "$Name.exe"
    @"
@echo off
"$target" %*
"@ | Set-Content -Path $shim -Encoding ASCII -Force
}

function New-Shortcut([string]$Path, [string]$Target, [string]$WorkingDir, [string]$Description) {
    $WshShell = New-Object -ComObject WScript.Shell
    $sc = $WshShell.CreateShortcut($Path)
    $sc.TargetPath = $Target
    $sc.WorkingDirectory = $WorkingDir
    $sc.Description = $Description
    $sc.Save()
}

# --- License ---
if (-not (Show-Eula -Accepted:$AcceptLicense)) {
    Write-Host "Installation cancelled. You must accept the license to install."
    exit 1
}

# --- Locate binaries (release zip root or build\Release) ---
$BuildDir = Join-Path $Root "build\Release"
if (Test-Path (Join-Path $Root "wp-engine.exe")) {
    $BuildDir = $Root
} elseif (-not $SkipBuild -and -not (Test-Path (Join-Path $BuildDir "wp-engine.exe"))) {
    Write-Host "Build output not found. Running build.ps1..."
    & "$Root\build.ps1"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

foreach ($exe in @("wp-engine.exe", "wp-cli.exe", "wp-ui.exe")) {
    if (-not (Test-Path (Join-Path $BuildDir $exe))) {
        Write-Error "Missing $exe in $BuildDir. Run .\build.ps1 first or use a release zip with prebuilt binaries."
    }
}

# --- Install files ---
Write-Host "Installing Wallpapi to $InstallRoot"
New-Item -ItemType Directory -Force -Path $InstallRoot | Out-Null
$WallDir = Join-Path $InstallRoot "wallpapers"
New-Item -ItemType Directory -Force -Path $WallDir | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $InstallRoot "docs") | Out-Null

foreach ($exe in @("wp-engine.exe", "wp-cli.exe", "wp-ui.exe")) {
    Copy-Item (Join-Path $BuildDir $exe) $InstallRoot -Force
}

# Config: preserve existing user config on upgrade
$configDest = Join-Path $InstallRoot "config.toml"
if (-not (Test-Path $configDest)) {
    if (Test-Path (Join-Path $Root "config.toml")) {
        Copy-Item (Join-Path $Root "config.toml") $configDest -Force
    } elseif (Test-Path (Join-Path $Root "config.toml.example")) {
        Copy-Item (Join-Path $Root "config.toml.example") $configDest -Force
    }
}
Copy-Item (Join-Path $Root "config.toml.example") (Join-Path $InstallRoot "config.toml.example") -Force -ErrorAction SilentlyContinue

if (Test-Path (Join-Path $Root "wallpapers")) {
    Copy-Item (Join-Path $Root "wallpapers\*") $WallDir -Recurse -Force -ErrorAction SilentlyContinue
}

@("LICENSE", "VERSION", "README.md", "CHANGELOG.md", "install.ps1", "uninstall.ps1", "run-engine.ps1", "restart-engine.ps1") | ForEach-Object {
    $src = Join-Path $Root $_
    if (Test-Path $src) { Copy-Item $src $InstallRoot -Force }
}
@("docs\EULA.md", "docs\THIRD_PARTY_NOTICES.md", "docs\INSTALL.md") | ForEach-Object {
    $src = Join-Path $Root $_
    if (Test-Path $src) { Copy-Item $src (Join-Path $InstallRoot "docs") -Force }
}

# --- Shortcuts & PATH ---
if (-not $SkipStartup) {
    $StartupFolder = [Environment]::GetFolderPath("Startup")
    New-Shortcut (Join-Path $StartupFolder "Wallpapi.lnk") `
        (Join-Path $InstallRoot "wp-engine.exe") $InstallRoot "Wallpapi wallpaper engine"
}

if (-not $SkipDesktopShortcut) {
    $Desktop = [Environment]::GetFolderPath("Desktop")
    New-Shortcut (Join-Path $Desktop "Wallpapi.lnk") `
        (Join-Path $InstallRoot "wp-ui.exe") $InstallRoot "Wallpapi control panel"
}

Add-ToUserPath $InstallRoot
Install-CommandShim "wp-cli" $InstallRoot
Install-CommandShim "wp-ui" $InstallRoot
Broadcast-EnvironmentChange

$env:Path = [Environment]::GetEnvironmentVariable("Path", "Machine") + ";" +
            [Environment]::GetEnvironmentVariable("Path", "User")

# --- Verify ---
Write-Host ""
Write-Host "Verifying installation..."
$cli = Join-Path $InstallRoot "wp-cli.exe"
& $cli help | Out-Null
if ($LASTEXITCODE -ne 0) { Write-Warning "wp-cli help returned exit code $LASTEXITCODE" }

$version = "unknown"
$verFile = Join-Path $InstallRoot "VERSION"
if (Test-Path $verFile) { $version = (Get-Content $verFile -Raw).Trim() }

Write-Host ""
Write-Host "============================================================"
Write-Host "  Wallpapi $version installed successfully"
Write-Host "============================================================"
Write-Host "  Location:  $InstallRoot"
Write-Host "  CLI:       wp-cli  (new terminal after install)"
Write-Host "  UI:        wp-ui   or desktop shortcut"
Write-Host "  Engine:    wp-engine (startup shortcut if enabled)"
Write-Host ""
Write-Host "  Start engine:  wp-engine   or reboot / use startup shortcut"
Write-Host "  Then run:      wp-cli status"
Write-Host "                 wp-ui"
Write-Host ""
Write-Host "  Uninstall:     $InstallRoot\uninstall.ps1"
Write-Host "============================================================"
