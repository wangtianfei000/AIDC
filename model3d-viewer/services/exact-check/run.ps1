$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$python = Join-Path $root 'FreeCAD-runtime/FreeCAD_1.1.3-Windows-x86_64-py311/bin/python.exe'
$cad = Join-Path $root 'samples/jingang-10axis-system-design/layout/jingang-10axis-layout.FCStd'
& $python (Join-Path $PSScriptRoot 'src/server.py') --cad $cad --port 47331
