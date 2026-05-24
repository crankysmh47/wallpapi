# Builds a portable Windows zip package ready to distribute or install.
param(
    [string]$Version = $(if (Test-Path "VERSION") { (Get-Content "VERSION" -Raw).Trim() } else { "0.0.0" }),
    [string]$OutDir = "dist"
)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
Set-Location $Root

Write-Host "Wallpapi packaging v$Version"
Write-Host ""

if (-not (Test-Path "build\Release\wp-engine.exe")) {
    Write-Host "Building Release..."
    & "$Root\build.ps1"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$BuildDir = Join-Path $Root "build\Release"
$StageName = "Wallpapi-$Version-win64"
$Stage = Join-Path (Join-Path $Root $OutDir) $StageName

if (Test-Path $Stage) {
    Remove-Item -Recurse -Force $Stage
}
New-Item -ItemType Directory -Force -Path $Stage | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $Stage "wallpapers") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $Stage "docs") | Out-Null

$Binaries = @("wp-engine.exe", "wp-cli.exe", "wp-ui.exe")
foreach ($bin in $Binaries) {
    $src = Join-Path $BuildDir $bin
    if (-not (Test-Path $src)) {
        Write-Error "Missing build output: $src"
    }
    Copy-Item $src $Stage -Force
}

Copy-Item "config.toml.example" (Join-Path $Stage "config.toml.example") -Force
if (-not (Test-Path (Join-Path $Stage "config.toml"))) {
    Copy-Item "config.toml.example" (Join-Path $Stage "config.toml") -Force
}

@(
    "install.ps1", "uninstall.ps1", "run-engine.ps1", "restart-engine.ps1",
    "LICENSE", "VERSION", "README.md", "CHANGELOG.md", "GETTING_STARTED.txt"
) | ForEach-Object {
    if (Test-Path $_) { Copy-Item $_ $Stage -Force }
}

@("docs\EULA.md", "docs\THIRD_PARTY_NOTICES.md", "docs\INSTALL.md") | ForEach-Object {
    if (Test-Path $_) {
        Copy-Item $_ (Join-Path $Stage "docs") -Force
    }
}

if (Test-Path "wallpapers") {
    Copy-Item "wallpapers\*" (Join-Path $Stage "wallpapers") -Recurse -Force -ErrorAction SilentlyContinue
}

$ZipPath = Join-Path (Join-Path $Root $OutDir) "$StageName.zip"
if (Test-Path $ZipPath) { Remove-Item -Force $ZipPath }
Compress-Archive -Path $Stage -DestinationPath $ZipPath -Force

Write-Host ""
Write-Host "Package created:"
Write-Host "  Folder: $Stage"
Write-Host "  Zip:    $ZipPath"
Write-Host ""
Write-Host "To install: extract the zip, then run install.ps1"
