$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
python (Join-Path $root "tools/aidc_modelica.py") @args
exit $LASTEXITCODE
