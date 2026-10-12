$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
python (Join-Path $root "modelica-browser/tests/verify_p3_tree_generator.py")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
