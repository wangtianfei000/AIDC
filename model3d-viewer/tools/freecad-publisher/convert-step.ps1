param(
  [Parameter(Mandatory = $true)][string]$InputPath,
  [Parameter(Mandatory = $true)][string]$OutputPath
)
$ErrorActionPreference = 'Stop'
$aidcRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$freeCadCmd = Join-Path $aidcRoot 'FreeCAD-runtime/FreeCAD_1.1.3-Windows-x86_64-py311/bin/freecadcmd.exe'
$converter = Join-Path $PSScriptRoot 'src/publish_step_generic.py'
if (-not (Test-Path -LiteralPath $freeCadCmd)) { throw "FreeCADCmd not found: $freeCadCmd" }
$source = (Resolve-Path -LiteralPath $InputPath).Path
$output = [System.IO.Path]::GetFullPath($OutputPath)
$env:AIDC_STEP_SOURCE = $source
$env:AIDC_STEP_OUTPUT = $output
$env:AIDC_STEP_REPORT = [System.IO.Path]::ChangeExtension($output, '.report.json')
& $freeCadCmd -c "import os,runpy,sys; sys.path.insert(0,os.path.dirname(r'$($converter.Replace('\','/'))')); runpy.run_path(r'$($converter.Replace('\','/'))',run_name='__main__')"
if ($LASTEXITCODE -ne 0) { throw "STEP conversion failed with exit code $LASTEXITCODE" }
Write-Host "Generic STEP GLB created: $output"

