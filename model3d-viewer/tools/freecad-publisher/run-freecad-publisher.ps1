$ErrorActionPreference = 'Stop'

$aidcRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$config = Join-Path $aidcRoot 'samples/jingang-10axis-system-design/model3d/publish.config.json'
if (-not (Test-Path -LiteralPath $config)) { throw "Publish config not found: $config" }
$model3dRoot = Split-Path -Parent $config
& (Join-Path $aidcRoot 'bin\aidc.cmd') model3d publish $config (Join-Path $model3dRoot 'generated')
if ($LASTEXITCODE -ne 0) { throw "AIDC publisher failed with exit code $LASTEXITCODE" }

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
