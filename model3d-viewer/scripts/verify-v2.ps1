$ErrorActionPreference = 'Stop'

$workspace = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

Push-Location $workspace
try {
  npm run test:model3d
  if ($LASTEXITCODE -ne 0) { throw "运动学内核与契约测试失败: $LASTEXITCODE" }

  npm run build:model3d
  if ($LASTEXITCODE -ne 0) { throw "运动学浏览生产构建失败: $LASTEXITCODE" }
}
finally {
  Pop-Location
}

Write-Host 'V2 verification passed: servo drive and kinematics browser'
