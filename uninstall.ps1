# Removes Wallpapi from the current user profile (PATH, shims, shortcuts, install folder).
param(
    [string]$InstallRoot = $(Join-Path $env:LOCALAPPDATA "Programs\Wallpapi")
)

$ErrorActionPreference = "Stop"

function Remove-FromUserPath([string]$Dir) {
    $envKey = "HKCU:\Environment"
    $current = (Get-ItemProperty -Path $envKey -Name "Path" -ErrorAction SilentlyContinue).Path
    if (-not $current) { return }
    $parts = $current -split ";" | Where-Object { $_ -and $_.Trim() -ne "" -and $_.TrimEnd('\') -ne $Dir.TrimEnd('\') }
    $new = ($parts) -join ";"
    Set-ItemProperty -Path $envKey -Name "Path" -Value $new
}

function Remove-Shim([string]$Name) {
    $shim = Join-Path $env:LOCALAPPDATA "Microsoft\WindowsApps\$Name.cmd"
    if (Test-Path $shim) { Remove-Item -Force $shim }
}

function Remove-ShortcutIfExists([string]$Path) {
    if (Test-Path $Path) { Remove-Item -Force $Path }
}

Write-Host "Uninstalling Wallpapi from $InstallRoot"

$cli = Join-Path $InstallRoot "wp-cli.exe"
if (Test-Path $cli) {
    try { & $cli stop 2>$null | Out-Null } catch { }
    Start-Sleep -Milliseconds 500
}

$StartupFolder = [Environment]::GetFolderPath("Startup")
$Desktop = [Environment]::GetFolderPath("Desktop")
Remove-ShortcutIfExists (Join-Path $StartupFolder "Wallpapi.lnk")
Remove-ShortcutIfExists (Join-Path $Desktop "Wallpapi.lnk")

Remove-Shim "wp-cli"
Remove-Shim "wp-ui"
Remove-FromUserPath $InstallRoot

if (Test-Path $InstallRoot) {
    Remove-Item -Recurse -Force $InstallRoot
}

Write-Host ""
Write-Host "Uninstall complete. Open a new terminal for PATH changes to apply."
