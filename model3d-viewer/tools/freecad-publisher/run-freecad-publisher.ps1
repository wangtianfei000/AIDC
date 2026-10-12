$ErrorActionPreference = 'Stop'

$aidcRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$freeCadCmd = Join-Path $aidcRoot 'FreeCAD-runtime/FreeCAD_1.1.3-Windows-x86_64-py311/bin/freecadcmd.exe'
$publisher = Join-Path $PSScriptRoot 'src/publish_model.py'
$config = Join-Path $aidcRoot 'samples/jingang-10axis-system-design/model3d/publish.config.json'

if (-not (Test-Path -LiteralPath $freeCadCmd)) { throw "FreeCADCmd not found: $freeCadCmd" }
if (-not (Test-Path -LiteralPath $config)) { throw "Publish config not found: $config" }

$env:AIDC_MODEL3D_PUBLISHER = $publisher.Replace('\', '/')
$env:AIDC_MODEL3D_CONFIG = $config.Replace('\', '/')
& $freeCadCmd -c "import os,runpy; runpy.run_path(os.environ['AIDC_MODEL3D_PUBLISHER'], run_name='__main__')"
if ($LASTEXITCODE -ne 0) { throw "FreeCAD publisher failed with exit code $LASTEXITCODE" }

$model3dRoot = Split-Path -Parent $config
$assemblySequence = Join-Path $model3dRoot 'assembly.sequence.json'
$generatedSequence = Join-Path $model3dRoot 'generated/assembly.sequence.json'
if (Test-Path -LiteralPath $assemblySequence) {
  Copy-Item -LiteralPath $assemblySequence -Destination $generatedSequence -Force
}
$posePresets = Join-Path $model3dRoot 'pose-presets.json'
$generatedPosePresets = Join-Path $model3dRoot 'generated/pose-presets.json'
if (Test-Path -LiteralPath $posePresets) {
  Copy-Item -LiteralPath $posePresets -Destination $generatedPosePresets -Force
}
