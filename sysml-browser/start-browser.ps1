param(
    [int]$WebPort = 4173,
    [int]$RuntimePort = 47321
)

$ErrorActionPreference = 'Stop'
$browserRoot = $PSScriptRoot
$runtimeScript = Join-Path $browserRoot '..\sysml-parser-runtime\run-runtime.ps1'
$runtimeProcess = $null

function Test-Runtime {
    try {
        $response = Invoke-WebRequest -UseBasicParsing -Uri "http://127.0.0.1:$RuntimePort/health" -TimeoutSec 1
        return $response.StatusCode -eq 200
    } catch {
        return $false
    }
}

if (-not (Test-Path (Join-Path $browserRoot 'dist\index.html'))) {
    Push-Location $browserRoot
    try { npm run build } finally { Pop-Location }
}

if (-not (Test-Runtime)) {
    $runtimeProcess = Start-Process -FilePath 'powershell.exe' -WindowStyle Hidden -PassThru -ArgumentList @(
        '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $runtimeScript, '-Port', $RuntimePort
    )
    for ($attempt = 0; $attempt -lt 30 -and -not (Test-Runtime); $attempt++) {
        Start-Sleep -Milliseconds 250
    }
    if (-not (Test-Runtime)) {
        if ($runtimeProcess -and -not $runtimeProcess.HasExited) { Stop-Process -Id $runtimeProcess.Id }
        throw "Parser Runtime failed to start on port $RuntimePort."
    }
}

Write-Host "AIDC SysML Browser: http://127.0.0.1:$WebPort/"
Write-Host 'Press Ctrl+C to stop.'

Push-Location $browserRoot
try {
    & (Join-Path $browserRoot 'node_modules\.bin\vite.cmd') preview --host 127.0.0.1 --port $WebPort
} finally {
    Pop-Location
    if ($runtimeProcess -and -not $runtimeProcess.HasExited) {
        Stop-Process -Id $runtimeProcess.Id
    }
}
