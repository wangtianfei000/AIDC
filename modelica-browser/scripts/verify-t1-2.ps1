[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = New-Object System.Text.UTF8Encoding($false)
$OutputEncoding = [Console]::OutputEncoding
$modelicaBrowser = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$aidcRoot = (Resolve-Path (Join-Path $modelicaBrowser '..')).Path
$repoRoot = (Resolve-Path (Join-Path $aidcRoot '..')).Path
$python = Join-Path $repoRoot 'tools\openmodelica\fmpy-venv\Scripts\python.exe'
$test = Join-Path $modelicaBrowser 'tests\verify_t1_2.py'
& $python $test
exit $LASTEXITCODE
