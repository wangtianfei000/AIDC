$ErrorActionPreference = 'Stop'

$workspace = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Push-Location $workspace
try {
  npm run test:model3d
  if ($LASTEXITCODE -ne 0) { throw "三维模型契约与运行时测试失败: $LASTEXITCODE" }
  npm run build:model3d
  if ($LASTEXITCODE -ne 0) { throw "三维浏览器生产构建失败: $LASTEXITCODE" }
}
finally {
  Pop-Location
}

Write-Host 'V1 verification passed: standalone 3D browser MVP'
