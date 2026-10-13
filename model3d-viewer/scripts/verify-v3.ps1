$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$freecadPython = Join-Path (Split-Path $root -Parent) 'tools/freecad/1.1.3/bin/python.exe'
Push-Location $root
try {
  npm run test:model3d
  npm run build:model3d
  & $freecadPython 'model3d-viewer/services/exact-check/tests/verify_exact_check.py'
} finally { Pop-Location }
