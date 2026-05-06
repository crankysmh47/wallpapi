$ErrorActionPreference = "Stop"

function Remove-FromUserPath([string]$Dir) {
    $envKey = "HKCU:\Environment"
    $current = (Get-ItemProperty -Path $envKey -Name "Path" -ErrorAction SilentlyContinue).Path
    if (-not $current) { return }
    $parts = $current -split ";" | Where-Object { $_ -and $_.Trim() -ne "" -and $_ -ne $Dir }
    $new = ($parts) -join ";"
    Set-ItemProperty -Path $envKey -Name "Path" -Value $new
}

function Remove-StartupShortcut() {
    $StartupFolder = [System.Environment]::GetFolderPath("Startup")
    $ShortcutPath = Join-Path $StartupFolder "Wallpapi.lnk"
    if (Test-Path $ShortcutPath) {
        Remove-Item -Force $ShortcutPath
    }
}

$InstallRoot = Join-Path $env:LOCALAPPDATA "Programs\Wallpapi"
Write-Host "Uninstalling from $InstallRoot"

# Best-effort stop
$cli = Join-Path $InstallRoot "wp-cli.exe"
if (Test-Path $cli) {
    try { & $cli stop | Out-Null } catch { }
}

Remove-StartupShortcut
Remove-FromUserPath $InstallRoot

if (Test-Path $InstallRoot) {
    Remove-Item -Recurse -Force $InstallRoot
}

Write-Host "------------------------------------------------"
Write-Host "Uninstall complete."
Write-Host "Note: PATH changes apply to new terminals only."
Write-Host "------------------------------------------------"

