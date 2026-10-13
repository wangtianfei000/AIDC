[CmdletBinding()]
param(
  [string]$InstallRoot = (Join-Path $env:LOCALAPPDATA 'Programs\AIDC'),
  [switch]$RemoveRuntimePacks,
  [switch]$KeepHostIntegrations
)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $InstallRoot)) { Write-Output 'AIDC is not installed'; exit 0 }
if (-not $KeepHostIntegrations) {
  $aidc = Join-Path $InstallRoot 'bin\aidc.exe'
  if (Test-Path -LiteralPath $aidc) {
    foreach ($hostName in @('codex', 'cursor', 'mcp')) {
      try {
        & $aidc integrate $hostName --uninstall --json | Out-Null
        if ($LASTEXITCODE) { Write-Warning "Unable to remove $hostName integration; use the preserved state backup to restore it manually." }
      } catch {
        Write-Warning "Unable to remove $hostName integration: $($_.Exception.Message)"
      }
    }
  }
}
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
