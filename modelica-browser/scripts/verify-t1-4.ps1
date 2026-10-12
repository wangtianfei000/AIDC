[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
[Console]::OutputEncoding=New-Object System.Text.UTF8Encoding($false)
$modelicaBrowser=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$aidcRoot=(Resolve-Path (Join-Path $modelicaBrowser '..')).Path
$repoRoot=(Resolve-Path (Join-Path $aidcRoot '..')).Path
$python=Join-Path $repoRoot 'tools\openmodelica\fmpy-venv\Scripts\python.exe'
Push-Location $aidcRoot
try {
  npm run build:modelica
  if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
  & $python (Join-Path $modelicaBrowser 'tests\verify_t1_4.py')
  exit $LASTEXITCODE
} finally { Pop-Location }
