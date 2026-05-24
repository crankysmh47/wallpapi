# Smoke test: build, install to a temp directory, start engine, exercise CLI, uninstall.
param(
    [string]$InstallRoot = $(Join-Path $env:TEMP "WallpapiTest_$([Guid]::NewGuid().ToString('N').Substring(0, 8))")
)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
Set-Location $Root

Write-Host "Wallpapi install smoke test"
Write-Host "  Test install root: $InstallRoot"
Write-Host ""

$engineProc = $null

try {
    if (-not (Test-Path "build\Release\wp-engine.exe")) {
        Write-Host "[1/6] Building..."
        & "$Root\build.ps1"
          if ($LASTEXITCODE -ne 0) { throw "build.ps1 failed with exit $LASTEXITCODE" }
    } else {
        Write-Host "[1/6] Using existing build\Release"
    }

    Write-Host "[2/6] Installing to temp directory..."
    & "$Root\install.ps1" -AcceptLicense -InstallRoot $InstallRoot -SkipStartup -SkipDesktopShortcut
      if ($LASTEXITCODE -ne 0) { throw "install.ps1 failed with exit $LASTEXITCODE" }

    foreach ($exe in @("wp-engine.exe", "wp-cli.exe", "wp-ui.exe")) {
        if (-not (Test-Path (Join-Path $InstallRoot $exe))) {
            throw "Missing installed binary: $exe"
        }
    }
    if (-not (Test-Path (Join-Path $InstallRoot "LICENSE"))) {
        throw "LICENSE not copied to install directory"
    }
    if (-not (Test-Path (Join-Path $InstallRoot "docs\EULA.md"))) {
        throw "EULA not copied to install directory"
    }

    Write-Host "[3/6] Starting wp-engine..."
    $engineProc = Start-Process -FilePath (Join-Path $InstallRoot "wp-engine.exe") `
        -WorkingDirectory $InstallRoot -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 4

    if ($engineProc.HasExited) {
        throw "wp-engine exited early with code $($engineProc.ExitCode)"
    }

    $cli = Join-Path $InstallRoot "wp-cli.exe"

    Write-Host "[4/6] wp-cli status..."
    $status = & $cli status 2>&1 | Out-String
    Write-Host $status
    if ($status -notmatch "OK status") { throw "Unexpected status response: $status" }

    Write-Host "[5/6] wp-cli list..."
    $list = & $cli list 2>&1 | Out-String
    Write-Host $list
    if ($LASTEXITCODE -ne 0) { throw "wp-cli list failed" }

    Write-Host "[6/6] wp-cli config get..."
    $cfg = & $cli config get 2>&1 | Out-String
    Write-Host $cfg
    if ($cfg -notmatch "OK config") { throw "Unexpected config response: $cfg" }

    Write-Host ""
    Write-Host "PASS: Install smoke test succeeded."
}
catch {
    Write-Host ""
    Write-Host "FAIL: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
finally {
    if ($engineProc -and -not $engineProc.HasExited) {
        Write-Host "Stopping test engine..."
        $stopCli = Join-Path $InstallRoot "wp-cli.exe"
        if (Test-Path $stopCli) {
            try { & $stopCli stop 2>$null | Out-Null } catch { }
            Start-Sleep -Seconds 1
        }
        if (-not $engineProc.HasExited) {
            Stop-Process -Id $engineProc.Id -Force -ErrorAction SilentlyContinue
        }
    }

    if (Test-Path $InstallRoot) {
        Write-Host "Removing test install at $InstallRoot"
        & "$Root\uninstall.ps1" -InstallRoot $InstallRoot
    }
}

exit 0
