$ErrorActionPreference = "Stop"

Write-Host "Configuring..."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Building..."
cmake --build build --config Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$OutDir = Join-Path (Get-Location) "build\Release"
$WallDir = Join-Path $OutDir "wallpapers"
New-Item -ItemType Directory -Force -Path $WallDir | Out-Null
Copy-Item -Path "config.toml" -Destination $OutDir -Force
$srcWall = Join-Path $env:LOCALAPPDATA "Programs\Wallpapi\wallpapers"
if (Test-Path $srcWall) {
    Copy-Item -Path "$srcWall\*" -Destination $WallDir -Recurse -Force -ErrorAction SilentlyContinue
} elseif (Test-Path ".\wallpapers") {
    Copy-Item -Path ".\wallpapers\*" -Destination $WallDir -Recurse -Force -ErrorAction SilentlyContinue
}
Write-Host "Build complete."
Write-Host "  Engine: $OutDir\wp-engine.exe"
Write-Host "  CLI:    $OutDir\wp-cli.exe"
Write-Host "  UI:     $OutDir\wp-ui.exe"
Write-Host ""
Write-Host "Run:  .\restart-engine.ps1"
Write-Host "Install: .\install.ps1"
