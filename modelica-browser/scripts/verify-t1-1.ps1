[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$modelicaBrowser = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$aidcRoot = (Resolve-Path (Join-Path $modelicaBrowser '..')).Path
$repoRoot = (Resolve-Path (Join-Path $aidcRoot '..')).Path
$python = Join-Path $repoRoot 'tools\openmodelica\fmpy-venv\Scripts\python.exe'
$tool = Join-Path $modelicaBrowser 'tools\project_tool.py'
$contractTests = Join-Path $modelicaBrowser 'packages\contracts\tests\validate_contracts.py'
$projects = @(
    (Join-Path $aidcRoot 'samples\modelica-t0-single-joint'),
    (Join-Path $aidcRoot 'samples\modelica-t1-dc-motor')
)
$started = Get-Date

function Get-Sha256Hex {
    param([string]$Path)
    $stream = [System.IO.File]::OpenRead($Path)
    try {
        $sha = [System.Security.Cryptography.SHA256]::Create()
        try { return ([System.BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-', '') }
        finally { $sha.Dispose() }
    }
    finally { $stream.Dispose() }
}

& $python $contractTests
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$canonicalRuns = @()
foreach ($project in $projects) { & $python $tool validate $project; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE } }

$scratchRoot = Join-Path $aidcRoot ('.codex-runtime\t1-1-clean-rebuild-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force -Path $scratchRoot | Out-Null
$cleanRuns = @()
foreach ($project in $projects) {
    $projectName = Split-Path $project -Leaf
    $copy = Join-Path $scratchRoot $projectName
    New-Item -ItemType Directory -Force -Path $copy | Out-Null
    Copy-Item -LiteralPath (Join-Path $project 'model-project.json') -Destination $copy
    foreach ($directory in @('modelica')) {
        $source = Join-Path $project $directory
        if (Test-Path -LiteralPath $source) { Copy-Item -LiteralPath $source -Destination $copy -Recurse }
    }
    if (Test-Path -LiteralPath (Join-Path $copy 'results')) {
        [Console]::Error.WriteLine("Clean copy unexpectedly contains results/: $copy")
        exit 30
    }
    & $python $tool validate $copy
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    $cleanRuns += $copy
}

$resultDir = Join-Path $modelicaBrowser 'results'
New-Item -ItemType Directory -Force -Path $resultDir | Out-Null
$reportPath = Join-Path $resultDir 'T1.1-契约与工程迁移验收报告.md'
$duration = [math]::Round(((Get-Date) - $started).TotalSeconds, 2)
$reportLines = @(
    '# T1.1：契约与工程迁移验收报告',
    '',
    '**结论：通过**',
    '',
    '- 契约：JSON Schema Draft 2020-12 + TypeScript 类型。',
    '- 工程：单关节闭环探针、直流电机开环样例。',
    '- 通用性：两个工程由同一个 project_tool.py 校验和运行。',
    '- 最小结构：工程仅包含 model-project.json、modelica/ 和 results/。',
    '- 干净副本：仅复制入口和模型即可通过校验。',
    '- 路径安全：Schema 拒绝绝对路径、反斜杠和 .. 越界。',
    "- 总耗时：$duration s。",
    '',
    '## 已验证工程',
    ''
)
$reportLines += $projects | ForEach-Object { '- `' + $_ + '`' }
$reportLines += @('', '## 无生成物副本重建', '', "副本根目录：$scratchRoot", '')
$reportLines += $cleanRuns | ForEach-Object { '- `' + $_ + '`' }
$reportLines += @('', '该验收仅表示 T1.1 契约和工程迁移完成；Project Server 与浏览器界面仍属于 T1.2、T1.3。', '')
$report = $reportLines -join "`n"
$report | Set-Content -LiteralPath $reportPath -Encoding utf8
Write-Host "T1.1 passed. Report: $reportPath"
