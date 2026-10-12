$ErrorActionPreference = 'Stop'

$workspace = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$stepSource = Join-Path $workspace 'FreeCAD/data/tests/Step/as1-ac-214_small.stp'
$artifactDir = Join-Path $workspace 'model3d-viewer/test-artifacts/e5'
$stepGlb = Join-Path $artifactDir 'third-party-step.glb'
$converter = Join-Path $workspace 'model3d-viewer/tools/freecad-publisher/convert-step.ps1'
$glbVerifier = Join-Path $workspace 'model3d-viewer/tools/freecad-publisher/tests/verify_generic_glb.py'

New-Item -ItemType Directory -Path $artifactDir -Force | Out-Null
& $converter -InputPath $stepSource -OutputPath $stepGlb
if ($LASTEXITCODE -ne 0) { throw "第三方 STEP 无配置转换失败: $LASTEXITCODE" }

python $glbVerifier $stepGlb
if ($LASTEXITCODE -ne 0) { throw "第三方 STEP GLB 结构验证失败: $LASTEXITCODE" }

Push-Location $workspace
try {
  npm run test:model3d
  if ($LASTEXITCODE -ne 0) { throw "E5 通用装配体运行时测试失败: $LASTEXITCODE" }
  npm run build:model3d
  if ($LASTEXITCODE -ne 0) { throw "三维浏览器生产构建失败: $LASTEXITCODE" }
}
finally {
  Pop-Location
}

Write-Host 'E5 verification passed: robot, coaxial shaft, housing, and configuration-free third-party STEP'

