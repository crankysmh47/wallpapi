# Check for prerequisites
if (-not (Get-Command "cmake" -ErrorAction SilentlyContinue)) {
    Write-Error "CMake not found! Please install it from https://cmake.org/download/"
    exit 1
}

# Check for a compiler
$CompilerFound = $false
if (Get-Command "cl" -ErrorAction SilentlyContinue) { $CompilerFound = $true; $Generator = "Visual Studio" }
elseif (Get-Command "gcc" -ErrorAction SilentlyContinue) { $CompilerFound = $true; $Generator = "MinGW Makefiles" }
elseif (Get-Command "ninja" -ErrorAction SilentlyContinue) { $CompilerFound = $true; $Generator = "Ninja" }

if (-not $CompilerFound) {
    Write-Warning "No C++ compiler (MSVC, GCC, or Ninja) found in PATH."
    Write-Host "Please install Visual Studio (with C++ Desktop development) or MinGW."
    # We will try to run cmake anyway as it might find a compiler we didn't
}

# Ensure build directory is clean
if (Test-Path "build") { 
    Write-Host "Cleaning existing build directory..."
    Remove-Item -Recurse -Force build 
}

# Create fresh build folder
New-Item -ItemType Directory -Force -Path "build" | Out-Null
Set-Location "build"

Write-Host "Configuring project with CMake..."
if ($Generator) {
    cmake .. -G "$Generator" -DCMAKE_BUILD_TYPE=Release
} else {
    cmake .. -DCMAKE_BUILD_TYPE=Release
}

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
Write-Host "SUCCESS! Wallpapi is built."
Write-Host "Binary location: build\Release\wp-engine.exe"
Set-Location ..
