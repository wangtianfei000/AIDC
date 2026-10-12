$ErrorActionPreference = 'Stop'

$workspace = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$sample = Join-Path $workspace 'samples\jingang-10axis-system-design'
$publisher = Join-Path $workspace 'model3d-viewer\tools\freecad-publisher'
$generated = Join-Path $sample 'model3d\generated'
$release = Join-Path $sample 'model3d\release\jingang-native10-p4.robot3d.zip'

& (Join-Path $publisher 'run-freecad-publisher.ps1')
if ($LASTEXITCODE -ne 0) { throw "FreeCAD 发布失败: $LASTEXITCODE" }

python (Join-Path $publisher 'src\validate_model_package.py') `
  --manifest (Join-Path $generated 'model3d.manifest.json') `
  --config (Join-Path $sample 'model3d\publish.config.json') `
  --expected (Join-Path $sample 'model3d\expected\neutral-body-poses.json') `
  --neutral-frame (Join-Path $sample 'model3d\simulation\neutral-frame.json') `
  --assembly (Join-Path $sample 'model3d\assembly.sequence.json') `
  --report (Join-Path $generated 'validation-report.json')
if ($LASTEXITCODE -ne 0) { throw "模型包验证失败: $LASTEXITCODE" }

Push-Location $workspace
try {
  npm run test:model3d
  if ($LASTEXITCODE -ne 0) { throw "模型测试失败: $LASTEXITCODE" }
  npm run build:model3d
  if ($LASTEXITCODE -ne 0) { throw "技术样机构建失败: $LASTEXITCODE" }
}
finally {
  Pop-Location
}

python (Join-Path $publisher 'src\package_release.py') `
  --sample $sample `
  --contracts (Join-Path $workspace 'model3d-viewer\packages\contracts\schemas') `
  --output $release
if ($LASTEXITCODE -ne 0) { throw "发布包生成失败: $LASTEXITCODE" }

Write-Host "V0 verification passed: $release"
