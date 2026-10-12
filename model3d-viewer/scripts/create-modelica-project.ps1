param(
  [Parameter(Mandatory = $true)][string]$PublishedModel,
  [Parameter(Mandatory = $true)][string]$Output,
  [switch]$Force
)
$ErrorActionPreference = "Stop"
$aidcRoot = Split-Path -Parent $PSScriptRoot
$aidcRoot = Split-Path -Parent $aidcRoot
$cli = Join-Path $aidcRoot "modelica-browser/tools/aidc_modelica.py"
$arguments = @($cli, "generate", $PublishedModel, $Output, "--profile", "auto", "--migrate")
if ($Force) { $arguments += "--force" }
python @arguments
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "Modelica project created. Open it with modelica-browser/scripts/start-project-server.ps1 -Project '$Output'."
