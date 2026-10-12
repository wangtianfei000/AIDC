[CmdletBinding()]
param([switch]$InstallBuildDependency)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if ($InstallBuildDependency) { python -m pip install --requirement (Join-Path $root 'distribution\requirements-build.txt') }

function Build-OneFile([string]$Name, [string]$Entry, [string[]]$Paths) {
  $arguments = @('-m','PyInstaller','--noconfirm','--clean','--onefile','--name',$Name)
  foreach ($path in $Paths) { $arguments += @('--paths', (Join-Path $root $path)) }
  $arguments += (Join-Path $root $Entry)
  & python @arguments
  if ($LASTEXITCODE -ne 0) { throw "$Name build failed with exit code $LASTEXITCODE" }
}

Build-OneFile 'aidc' 'aidc.py' @('.')
Build-OneFile 'aidc-mcp' 'aidc_mcp.py' @('.')
Build-OneFile 'aidc-modelica-server' 'modelica-browser/services/project-server/server.py' @('modelica-browser/services/project-server','modelica-browser/tools','modelica-browser/packages/robot-ir/src','modelica-browser/packages/assembly-package/src')
Build-OneFile 'aidc-modelica-tools' 'modelica-browser/tools/aidc_modelica.py' @('modelica-browser/tools','modelica-browser/packages/robot-ir/src','modelica-browser/packages/assembly-package/src')
$exportArgs = @('-m','PyInstaller','--noconfirm','--clean','--onefile','--name','aidc-model3d-export',
  '--exclude-module','matplotlib','--exclude-module','IPython','--exclude-module','pytest','--exclude-module','scipy','--exclude-module','pandas','--exclude-module','numba','--exclude-module','sklearn','--exclude-module','skimage','--exclude-module','cv2','--exclude-module','av','--exclude-module','imageio','--exclude-module','sqlalchemy','--exclude-module','openpyxl','--exclude-module','tkinter',
  '--paths',(Join-Path $root 'model3d-viewer/tools/freecad-publisher/src'),
  '--paths',(Join-Path $root 'modelica-browser/packages/robot-ir/src'),
  '--paths',(Join-Path $root 'modelica-browser/packages/assembly-package/src'),
  (Join-Path $root 'model3d-viewer/tools/freecad-publisher/src/export_modelica_assembly.py'))
& python @exportArgs
if ($LASTEXITCODE -ne 0) { throw "aidc-model3d-export build failed with exit code $LASTEXITCODE" }


