# pio-diagnose.ps1 -- reset PlatformIO's ESP32 toolchain cache and capture a full build log.
#
# Usage (close VS Code first, then in a normal PowerShell window):
#   powershell -ExecutionPolicy Bypass -File C:\dev\stopwatch\scripts\pio-diagnose.ps1 -Reset
#
# -Reset deletes ONLY downloaded/cached components under %USERPROFILE%\.platformio
# (platforms, packages, tools, dist, .cache, idf-env.json). PlatformIO itself (penv)
# is kept; everything deleted is re-downloaded on the next build.
# Output: <project>\pio-build.log (git-ignored)

param([switch]$Reset)

$project = Split-Path -Parent $PSScriptRoot
$pioHome = Join-Path $env:USERPROFILE ".platformio"
$pio = Join-Path $pioHome "penv\Scripts\platformio.exe"
$log = Join-Path $project "pio-build.log"

if (-not (Test-Path $pio)) { Write-Host "PlatformIO not found at $pio"; exit 1 }

if ($Reset) {
    Write-Host "Removing cached PlatformIO components..."
    foreach ($p in "platforms", "packages", "tools", "dist", ".cache", "idf-env.json") {
        Remove-Item -Recurse -Force (Join-Path $pioHome $p) -ErrorAction SilentlyContinue
    }
    Remove-Item -Recurse -Force (Join-Path $project ".pio") -ErrorAction SilentlyContinue
}

$longPaths = (Get-ItemProperty 'HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem' -Name LongPathsEnabled -ErrorAction SilentlyContinue).LongPathsEnabled
$envVars = Get-ChildItem env: | Where-Object { $_.Name -match '^(IDF|PLATFORMIO|PYTHON|HTTPS?_PROXY|NO_PROXY)' } |
    ForEach-Object { "  $($_.Name)=$($_.Value)" }

$header = @(
    "=== pio-diagnose $(Get-Date -Format s) ==="
    "Windows: $([Environment]::OSVersion.VersionString)"
    "Project: $project"
    "PlatformIO: $(& $pio --version 2>&1)"
    "LongPathsEnabled: $longPaths"
    "Relevant env vars:"
) + $envVars + "=== pio run -e stopwatch -v ==="
Set-Content -Path $log -Value $header -Encoding utf8

Write-Host "Building (first run downloads ~1 GB and can take 10-20 min). Logging to $log"
Push-Location $project
cmd /c "`"$pio`" run -e stopwatch -v >> `"$log`" 2>&1"
$rc = $LASTEXITCODE
Pop-Location

Add-Content -Path $log -Value "=== exit code: $rc ===" -Encoding utf8
Write-Host "Done (exit code $rc). Log: $log"
