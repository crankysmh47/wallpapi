# Register Wallpapi for Startup
$EnginePath = (Get-Item ".\build\Release\wp-engine.exe").FullName
$StartupFolder = [System.Environment]::GetFolderPath("Startup")
$ShortcutPath = Join-Path $StartupFolder "Wallpapi.lnk"

if (-not (Test-Path $EnginePath)) {
    Write-Error "Could not find wp-engine.exe. Please run .\build.ps1 first."
    exit
}

$WshShell = New-Object -ComObject WScript.Shell
$Shortcut = $WshShell.CreateShortcut($ShortcutPath)
$Shortcut.TargetPath = $EnginePath
$Shortcut.WorkingDirectory = Split-Path $EnginePath -Parent
$Shortcut.Description = "Wallpapi Video Wallpaper Engine"
$Shortcut.Save()

Write-Host "------------------------------------------------"
Write-Host "SUCCESS! Wallpapi registered for startup."
Write-Host "Location: $ShortcutPath"
Write-Host "------------------------------------------------"
