$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$freecadPython = Join-Path $root 'FreeCAD-runtime/FreeCAD_1.1.3-Windows-x86_64-py311/bin/python.exe'
Push-Location $root
try {
  npm run test:model3d
  npm run build:model3d
  & $freecadPython 'model3d-viewer/services/exact-check/tests/verify_exact_check.py'
} finally { Pop-Location }
