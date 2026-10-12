[CmdletBinding()]
param(
  [string]$Source = (Join-Path $PSScriptRoot 'product'),
  [string]$InstallRoot = (Join-Path $env:LOCALAPPDATA 'Programs\AIDC'),
  [string[]]$RuntimePack = @()
)
$ErrorActionPreference = 'Stop'
$sourcePath = (Resolve-Path -LiteralPath $Source).Path
if (-not (Test-Path -LiteralPath (Join-Path $sourcePath 'bin\aidc.exe'))) { throw "Invalid AIDC product source: $sourcePath" }
$parent = Split-Path -Parent $InstallRoot
New-Item -ItemType Directory -Force -Path $parent | Out-Null
$stage = Join-Path $parent ('.aidc-install-' + [guid]::NewGuid().ToString('N'))
$backup = $null
try {
  Copy-Item -LiteralPath $sourcePath -Destination $stage -Recurse -Force
  if (Test-Path -LiteralPath $InstallRoot) {
    $backup = "$InstallRoot.previous"
    if (Test-Path -LiteralPath $backup) { Remove-Item -LiteralPath $backup -Recurse -Force }
    Move-Item -LiteralPath $InstallRoot -Destination $backup
    $existingRuntimes = Join-Path $backup 'runtimes'
    if (Test-Path -LiteralPath $existingRuntimes) {
      $stageRuntimes = Join-Path $stage 'runtimes'
      if (Test-Path -LiteralPath $stageRuntimes) { Remove-Item -LiteralPath $stageRuntimes -Recurse -Force }
      Move-Item -LiteralPath $existingRuntimes -Destination $stageRuntimes
    }
  }
  Move-Item -LiteralPath $stage -Destination $InstallRoot
  foreach ($packSpec in $RuntimePack) {
    $parts = $packSpec -split '=', 2
    if ($parts.Count -ne 2) { throw "RuntimePack must use dependencyId=path: $packSpec" }
    & (Join-Path $InstallRoot 'bin\aidc.exe') deps install $parts[0] --source $parts[1] --json
    if ($LASTEXITCODE) { throw "Runtime pack installation failed: $packSpec" }
  }
  [ordered]@{schemaVersion='aidc.install/1.0';installedAt=[DateTime]::UtcNow.ToString('o');installRoot=$InstallRoot;previous=$backup} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $InstallRoot 'install-manifest.json') -Encoding utf8
  Write-Output (Join-Path $InstallRoot 'bin\aidc.exe')
} catch {
  if ($backup -and (Test-Path -LiteralPath $backup)) {
    $backupRuntimes = Join-Path $backup 'runtimes'
    $stageRuntimes = Join-Path $stage 'runtimes'
    if ((Test-Path -LiteralPath $stageRuntimes) -and -not (Test-Path -LiteralPath $backupRuntimes)) {
      Move-Item -LiteralPath $stageRuntimes -Destination $backupRuntimes
    }
    if (Test-Path -LiteralPath $InstallRoot) {
      $installedRuntimes = Join-Path $InstallRoot 'runtimes'
      if ((Test-Path -LiteralPath $installedRuntimes) -and -not (Test-Path -LiteralPath $backupRuntimes)) {
        Move-Item -LiteralPath $installedRuntimes -Destination $backupRuntimes
      }
      Remove-Item -LiteralPath $InstallRoot -Recurse -Force
    }
    Move-Item -LiteralPath $backup -Destination $InstallRoot
  } elseif (Test-Path -LiteralPath $InstallRoot) {
    Remove-Item -LiteralPath $InstallRoot -Recurse -Force
  }
  if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
  throw
}
