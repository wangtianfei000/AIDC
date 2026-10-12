$ErrorActionPreference = "Stop"
$aidcRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
& (Join-Path $aidcRoot 'bin\aidc.cmd') modelica @args
exit $LASTEXITCODE
