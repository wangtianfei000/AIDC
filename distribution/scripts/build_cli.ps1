[CmdletBinding()]
param([switch]$InstallBuildDependency)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if ($InstallBuildDependency) {
    python -m pip install --requirement (Join-Path $root 'distribution\requirements-build.txt')
}
python -m PyInstaller --noconfirm --clean --onefile --name aidc --paths $root (Join-Path $root 'aidc.py')
if ($LASTEXITCODE -ne 0) { throw "aidc.exe 构建失败，退出码 $LASTEXITCODE" }
Write-Output (Join-Path $root 'dist\aidc.exe')
