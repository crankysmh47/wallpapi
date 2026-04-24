$Owner = "shinchiro"
$Repo = "mpv-winbuild-cmake"

# 1. Get latest release assets from GitHub API
Write-Host "Fetching latest release information from GitHub..."
$ApiUrl = "https://api.github.com/repos/$Owner/$Repo/releases/latest"
try {
    $Release = Invoke-RestMethod -Uri $ApiUrl
} catch {
    Write-Error "Failed to fetch release info. Check your internet connection."
    exit 1
}

# 2. Find the mpv-dev-x86_64-v3 asset
$Asset = $Release.assets | Where-Object { $_.name -like "mpv-dev-x86_64-v3-*" -and $_.name -notlike "*debug*" } | Select-Object -First 1

if (-not $Asset) {
    Write-Error "Could not find mpv-dev asset in the latest release."
    exit 1
}

$DownloadUrl = $Asset.browser_download_url
$FileName = $Asset.name

Write-Host "Downloading $FileName..."
Invoke-WebRequest -Uri $DownloadUrl -OutFile $FileName

# 3. Setup directory structure
Write-Host "Setting up deps directory..."
if (Test-Path "deps") { Remove-Item -Recurse -Force deps }
New-Item -ItemType Directory -Force -Path "deps/include" | Out-Null
New-Item -ItemType Directory -Force -Path "deps/lib" | Out-Null
if (Test-Path "temp_extract") { Remove-Item -Recurse -Force temp_extract }
New-Item -ItemType Directory -Force -Path "temp_extract" | Out-Null

# 4. Extract
Write-Host "Extracting binaries..."
tar -xf $FileName -C temp_extract

# 5. Move files (Flexible search)
Write-Host "Moving files to deps/..."
Get-ChildItem -Path "temp_extract" -Filter "mpv" -Recurse -Directory | ForEach-Object {
    if ($_.Parent.Name -eq "include") {
        Move-Item -Path $_.FullName -Destination "deps/include/" -Force
    }
}

Get-ChildItem -Path "temp_extract" -Filter "*.lib" -Recurse | ForEach-Object {
    Move-Item -Path $_.FullName -Destination "deps/lib/" -Force
}

Get-ChildItem -Path "temp_extract" -Filter "*.dll" -Recurse | ForEach-Object {
    Move-Item -Path $_.FullName -Destination "deps/lib/" -Force
}

Get-ChildItem -Path "temp_extract" -Filter "*.dll.a" -Recurse | ForEach-Object {
    Move-Item -Path $_.FullName -Destination "deps/lib/" -Force
}

# 6. Cleanup
Write-Host "Cleaning up..."
Remove-Item -Recurse -Force temp_extract
Remove-Item $FileName

Write-Host "------------------------------------------------"
Write-Host "Setup complete. libmpv binaries are in deps/."
Write-Host "You can now run .\build.ps1 to compile the project."
