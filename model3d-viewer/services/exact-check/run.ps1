$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$python = Join-Path (Split-Path $root -Parent) 'tools/freecad/1.1.3/bin/python.exe'
$cad = Join-Path $root 'samples/jingang-10axis-system-design/layout/jingang-10axis-layout.FCStd'
& $python (Join-Path $PSScriptRoot 'src/server.py') --cad $cad --port 47331
