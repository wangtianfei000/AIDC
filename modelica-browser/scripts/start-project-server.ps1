[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Project,
    [int]$Port = 4185
)

$ErrorActionPreference = 'Stop'
$modelicaBrowser = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$aidcRoot = (Resolve-Path (Join-Path $modelicaBrowser '..')).Path
$repoRoot = (Resolve-Path (Join-Path $aidcRoot '..')).Path
$python = Join-Path $repoRoot 'tools\openmodelica\fmpy-venv\Scripts\python.exe'
$server = Join-Path $modelicaBrowser 'services\project-server\server.py'
$projectPath = (Resolve-Path $Project).Path
& $python $server --project $projectPath --host 127.0.0.1 --port $Port
exit $LASTEXITCODE
