# Ensure build directory is clean
if (Test-Path "build") { 
    Write-Host "Cleaning existing build directory..."
    Remove-Item -Recurse -Force build 
}

# Create fresh build folder
New-Item -ItemType Directory -Force -Path "build" | Out-Null
Set-Location "build"

Write-Host "Configuring project with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed."
    Set-Location ..
    exit 1
}

Write-Host "Building project (Release)..."
cmake --build . --config Release

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed."
    Set-Location ..
    exit 1
}

# Copy the DLL to the output folder
Write-Host "Copying libmpv DLL to output directory..."
$Dll = Get-ChildItem -Path "../deps/lib" -Filter "*mpv*.dll" | Select-Object -First 1

if ($Dll) {
    $DestDir = if (Test-Path "Release") { "Release" } else { "." }
    Copy-Item $Dll.FullName -Destination "$DestDir/$($Dll.Name)"
    Write-Host "Copied $($Dll.Name) to $DestDir"
}
 else {
    Write-Warning "Could not find any mpv DLL in deps/lib to copy."
}

Write-Host "------------------------------------------------"
Write-Host "SUCCESS! The Wallpaper Engine Killer is built."
Write-Host "Binary location: build\Release\wp-engine.exe"
Set-Location ..
