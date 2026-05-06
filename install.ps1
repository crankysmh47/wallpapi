$ErrorActionPreference = "Stop"

function Add-ToUserPath([string]$Dir) {
    $envKey = "HKCU:\Environment"
    $current = (Get-ItemProperty -Path $envKey -Name "Path" -ErrorAction SilentlyContinue).Path
    if (-not $current) { $current = "" }
    $parts = $current -split ";" | Where-Object { $_ -and $_.Trim() -ne "" }
    if ($parts -contains $Dir) { return }
    $new = ($parts + $Dir) -join ";"
    Set-ItemProperty -Path $envKey -Name "Path" -Value $new
}

function Remove-FromUserPath([string]$Dir) {
    $envKey = "HKCU:\Environment"
    $current = (Get-ItemProperty -Path $envKey -Name "Path" -ErrorAction SilentlyContinue).Path
    if (-not $current) { return }
    $parts = $current -split ";" | Where-Object { $_ -and $_.Trim() -ne "" -and $_ -ne $Dir }
    $new = ($parts) -join ";"
    Set-ItemProperty -Path $envKey -Name "Path" -Value $new
}

function Register-StartupShortcut([string]$EnginePath, [string]$WorkingDir) {
    $StartupFolder = [System.Environment]::GetFolderPath("Startup")
    $ShortcutPath = Join-Path $StartupFolder "Wallpapi.lnk"

    $WshShell = New-Object -ComObject WScript.Shell
    $Shortcut = $WshShell.CreateShortcut($ShortcutPath)
    $Shortcut.TargetPath = $EnginePath
    $Shortcut.WorkingDirectory = $WorkingDir
    $Shortcut.Description = "Wallpapi Video Wallpaper Engine"
    $Shortcut.Save()
}

$InstallRoot = Join-Path $env:LOCALAPPDATA "Programs\Wallpapi"
Write-Host "Installing to $InstallRoot"

New-Item -ItemType Directory -Force -Path $InstallRoot | Out-Null

# Copy current folder contents into install root
$Here = (Get-Item ".").FullName
Copy-Item -Path (Join-Path $Here "*") -Destination $InstallRoot -Recurse -Force

$Engine = Join-Path $InstallRoot "wp-engine.exe"
$Cli = Join-Path $InstallRoot "wp-cli.exe"

if (-not (Test-Path $Engine)) { throw "wp-engine.exe not found in install payload." }
if (-not (Test-Path $Cli)) { throw "wp-cli.exe not found in install payload." }

Add-ToUserPath $InstallRoot
Register-StartupShortcut $Engine $InstallRoot

Write-Host "------------------------------------------------"
Write-Host "SUCCESS!"
Write-Host "- Installed to: $InstallRoot"
Write-Host "- Added to user PATH (new terminals only)"
Write-Host "- Startup shortcut created"
Write-Host "Run: wp-cli status"
Write-Host "------------------------------------------------"

