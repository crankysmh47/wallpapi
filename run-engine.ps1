# Run Wallpapi Engine
$EnginePath = ""
if (Test-Path ".\build\Release\wp-engine.exe") {
    $EnginePath = ".\build\Release\wp-engine.exe"
} elseif (Test-Path ".\build2\Release\wp-engine.exe") {
    $EnginePath = ".\build2\Release\wp-engine.exe"
}

if ($EnginePath -eq "") {
    Write-Error "Engine executable not found. Please build the project first."
    exit
}

Write-Host "Starting Wallpapi Engine from $EnginePath..."
Start-Process $EnginePath
