# Register startup + PATH without full install (uses build\Release)
$ErrorActionPreference = "Stop"
& (Join-Path $PSScriptRoot "install.ps1")
