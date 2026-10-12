$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
Push-Location $root
try {
  npm run verify:model3d
  if ($LASTEXITCODE -ne 0) { throw "模型包与装配序列验证失败: $LASTEXITCODE" }
} finally { Pop-Location }
Write-Host 'V4 verification passed: exploded view and assembly simulation'
