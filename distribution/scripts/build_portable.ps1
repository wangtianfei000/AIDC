[CmdletBinding()]
param([switch]$SkipFrontendBuild, [switch]$InstallBuildDependency)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if (-not $SkipFrontendBuild) {
  Push-Location $root
  try {
    & npm --workspace aidc-sysml-browser run build
    if ($LASTEXITCODE) { throw 'SysML browser build failed' }
    & npm --workspace @aidc/model3d-browser run build
    if ($LASTEXITCODE) { throw 'Model3D browser build failed' }
    & npm --workspace @aidc/modelica-browser-web run build
    if ($LASTEXITCODE) { throw 'Modelica browser build failed' }
  } finally { Pop-Location }
}
& (Join-Path $PSScriptRoot 'build_services.ps1') -InstallBuildDependency:$InstallBuildDependency
if ($LASTEXITCODE) { throw 'Service component build failed' }
python (Join-Path $PSScriptRoot 'assemble_portable.py')
if ($LASTEXITCODE) { throw 'Portable assembly failed' }


