[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Project,
    [string]$Scenario,
    [switch]$ValidateOnly
)

$ErrorActionPreference = 'Stop'
$modelicaBrowser = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$aidcRoot = (Resolve-Path (Join-Path $modelicaBrowser '..')).Path
$repoRoot = (Resolve-Path (Join-Path $aidcRoot '..')).Path
$python = Join-Path $repoRoot 'tools\openmodelica\fmpy-venv\Scripts\python.exe'
$tool = Join-Path $modelicaBrowser 'tools\project_tool.py'
$projectPath = (Resolve-Path $Project).Path

if (-not (Test-Path -LiteralPath $python)) {
    [Console]::Error.WriteLine("FMPy Python environment not found: $python")
    exit 10
}

if ($ValidateOnly) {
    & $python $tool validate $projectPath
    exit $LASTEXITCODE
}

$arguments = @($tool, 'run', $projectPath)
if ($Scenario) { $arguments += @('--scenario', $Scenario) }
& $python @arguments
exit $LASTEXITCODE
