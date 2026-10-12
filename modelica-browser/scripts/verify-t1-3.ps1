[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = New-Object System.Text.UTF8Encoding($false)
$OutputEncoding = [Console]::OutputEncoding
$modelicaBrowser = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$aidcRoot = (Resolve-Path (Join-Path $modelicaBrowser '..')).Path
$repoRoot = (Resolve-Path (Join-Path $aidcRoot '..')).Path
$python = Join-Path $repoRoot 'tools\openmodelica\fmpy-venv\Scripts\python.exe'
$test = Join-Path $modelicaBrowser 'tests\verify_t1_3.py'
Push-Location $aidcRoot
try {
    npm run build:modelica
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    & $python $test
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
