[CmdletBinding()]
param([string]$InstallRoot = (Join-Path $env:LOCALAPPDATA 'Programs\AIDC'), [switch]$RemoveRuntimePacks)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $InstallRoot)) { Write-Output 'AIDC is not installed'; exit 0 }
if (-not $RemoveRuntimePacks) {
  $runtime = Join-Path $InstallRoot 'runtimes'
  if (Test-Path -LiteralPath $runtime) {
    $preserved = Join-Path (Split-Path -Parent $InstallRoot) ('AIDC-runtimes-' + [DateTime]::UtcNow.ToString('yyyyMMddHHmmss'))
    Move-Item -LiteralPath $runtime -Destination $preserved
    Write-Output "Runtime packs preserved at $preserved"
  }
}
Remove-Item -LiteralPath $InstallRoot -Recurse -Force
Write-Output 'AIDC uninstalled; user projects and %LOCALAPPDATA%\AIDC\state were preserved.'
