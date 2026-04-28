# Register Wallpapi for Startup
$EnginePath = $null
$CommonPaths = @(
    ".\build\Release\wp-engine.exe",
    ".\build\Debug\wp-engine.exe",
    ".\build\wp-engine.exe",
    ".\wp-engine.exe"
)

foreach ($Path in $CommonPaths) {
    if (Test-Path $Path) {
        $EnginePath = (Get-Item $Path).FullName
        break
    }
}

$StartupFolder = [System.Environment]::GetFolderPath("Startup")
$DesktopFolder = [System.Environment]::GetFolderPath("Desktop")
$StartupShortcutPath = Join-Path $StartupFolder "Wallpapi.lnk"
$ChangeWallpaperShortcutPath = Join-Path $DesktopFolder "Change Wallpaper.lnk"

if (-not $EnginePath) {
    Write-Error "Could not find wp-engine.exe. Please run .\build.ps1 first."
    exit
}

$WshShell = New-Object -ComObject WScript.Shell

# 1. Startup Shortcut
$Shortcut = $WshShell.CreateShortcut($StartupShortcutPath)
$Shortcut.TargetPath = $EnginePath
$Shortcut.WorkingDirectory = (Get-Item ".").FullName
$Shortcut.Description = "Wallpapi Video Wallpaper Engine"
$Shortcut.Save()

# 2. Quick Change Shortcut on Desktop
$SelectorScript = (Get-Item ".\tools\select_wallpaper.ps1").FullName
$Shortcut = $WshShell.CreateShortcut($ChangeWallpaperShortcutPath)
$Shortcut.TargetPath = "powershell.exe"
$Shortcut.Arguments = "-ExecutionPolicy Bypass -File `"$SelectorScript`""
$Shortcut.WorkingDirectory = (Get-Item ".").FullName
$Shortcut.Description = "Quickly change Wallpapi wallpaper"
$Shortcut.IconLocation = "shell32.dll, 137" # Video icon
$Shortcut.Save()

Write-Host "------------------------------------------------"
Write-Host "SUCCESS! Wallpapi registered for startup."
Write-Host "Desktop shortcut 'Change Wallpaper' created."
Write-Host "------------------------------------------------"
