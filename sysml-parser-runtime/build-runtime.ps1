[CmdletBinding()]
param([string]$Python = 'python')

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path $PSScriptRoot).Path
$build = Join-Path $root 'build'
$dist = Join-Path $root 'dist'

& $Python -c 'import PyInstaller; print(PyInstaller.__version__)'
if ($LASTEXITCODE -ne 0) { throw 'PyInstaller is required: python -m pip install pyinstaller' }
foreach ($path in @($build, $dist)) {
    if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Recurse -Force }
}

$env:PYTHONPATH = Join-Path $root 'src'
& $Python -m unittest discover -s (Join-Path $root 'tests') -p 'test_*.py' -v
if ($LASTEXITCODE -ne 0) { throw "Runtime contract tests failed: $LASTEXITCODE" }
& $Python -m PyInstaller --noconfirm --clean --onefile `
    --name 'aidc-sysml-parser-runtime' `
    --paths (Join-Path $root 'src') `
    --hidden-import 'aidc_sysml_runtime.analysis' `
    --hidden-import 'aidc_sysml_runtime.model' `
    --exclude-module 'PySide6' --exclude-module 'PyQt5' --exclude-module 'PyQt6' `
    --workpath (Join-Path $build 'pyinstaller') `
    --specpath $build `
    --distpath (Join-Path $build 'bin') `
    (Join-Path $root 'src\runtime_main.py')
if ($LASTEXITCODE -ne 0) { throw "Runtime build failed: $LASTEXITCODE" }

$bin = Join-Path $dist 'bin'; $schema = Join-Path $dist 'schema'; $doc = Join-Path $dist 'doc'; $library = Join-Path $dist 'library'
New-Item -ItemType Directory -Force -Path $bin, $schema, $doc, $library | Out-Null
Copy-Item -LiteralPath (Join-Path $build 'bin\aidc-sysml-parser-runtime.exe') -Destination (Join-Path $bin 'aidc-sysml-parser-runtime.exe')
Copy-Item -LiteralPath (Join-Path $build 'bin\aidc-sysml-parser-runtime.exe') -Destination (Join-Path $bin 'aidc-sysml-parser-runtime-1.0.0.exe')
Copy-Item -LiteralPath (Join-Path $root '..\reference-parser-p0\model-snapshot.schema.json') -Destination $schema
Copy-Item -LiteralPath (Join-Path $root '..\reference-parser-p0\analysis-snapshot.schema.json') -Destination $schema
Copy-Item -LiteralPath (Join-Path $root 'README.md') -Destination $doc
Copy-Item -LiteralPath (Join-Path $root '..\reference-parser-p0\parser-contract.md') -Destination $doc
Copy-Item -LiteralPath (Join-Path $root 'run-runtime.ps1') -Destination $dist
Copy-Item -LiteralPath (Join-Path $root 'standard-library\2026-08\sysml.library') -Destination (Join-Path $library 'sysml.library') -Recurse
Copy-Item -LiteralPath (Join-Path $root 'standard-library\2026-08\LICENSE') -Destination $library
Copy-Item -LiteralPath (Join-Path $root 'standard-library\2026-08\README.md') -Destination $library
if (Get-ChildItem -LiteralPath $dist -Recurse -File | Where-Object { $_.Name -match '^(Qt|PySide|PyQt)' }) { throw 'Qt dependency detected in distribution' }
Write-Host "Qt-free SysML runtime created: $dist"
