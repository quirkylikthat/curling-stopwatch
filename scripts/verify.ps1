# verify.ps1 -- Windows version of scripts/verify.sh for local use.
#   powershell -ExecutionPolicy Bypass -File scripts\verify.ps1
# Runs: unit tests (if g++ is on PATH) and the PlatformIO firmware build.
# The full gate (format + Arduino CLI build too) runs in CI via verify.sh.

$ErrorActionPreference = "Stop"
$project = Split-Path -Parent $PSScriptRoot
$pio = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\platformio.exe"
if (-not (Test-Path $pio)) { Write-Host "PlatformIO not found at $pio"; exit 1 }

Push-Location $project
try {
    Write-Host "`n=== unit tests ==="
    if (Get-Command g++ -ErrorAction SilentlyContinue) {
        & $pio run -e native -t tests
        if ($LASTEXITCODE) { throw "unit tests failed" }
    } else {
        Write-Host "skipped (no g++ on PATH; install MSYS2 or WinLibs)"
    }

    Write-Host "`n=== firmware (PlatformIO) ==="
    & $pio run -e stopwatch      # never add -v: pioarduino 55.03.39 crashes in verbose mode
    if ($LASTEXITCODE) { throw "firmware build failed" }

    Write-Host "`nall local checks passed"
} finally {
    Pop-Location
}
