[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$PackageUrl, [Parameter(Mandatory=$true)][string]$Sha256, [string]$InstallRoot = (Join-Path $env:LOCALAPPDATA 'Programs\AIDC'))
$ErrorActionPreference = 'Stop'
$cache = Join-Path $env:LOCALAPPDATA 'AIDC\downloads'; New-Item -ItemType Directory -Force -Path $cache | Out-Null
$archive = Join-Path $cache 'aidc-online.zip'; $extract = Join-Path $cache 'aidc-online'
$offset = if (Test-Path -LiteralPath $archive) { (Get-Item -LiteralPath $archive).Length } else { 0 }
$request = [System.Net.HttpWebRequest]::Create($PackageUrl)
if ($offset -gt 0) { $request.AddRange($offset) }
$response = $request.GetResponse()
try {
  $append = $offset -gt 0 -and [int]$response.StatusCode -eq 206
  $mode = if ($append) { [System.IO.FileMode]::Append } else { [System.IO.FileMode]::Create }
  $output = [System.IO.File]::Open($archive, $mode, [System.IO.FileAccess]::Write, [System.IO.FileShare]::None)
  try { $response.GetResponseStream().CopyTo($output) } finally { $output.Dispose() }
} finally { $response.Dispose() }
$actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $archive).Hash.ToLowerInvariant()
if ($actual -ne $Sha256.ToLowerInvariant()) { throw "Core package SHA-256 mismatch: $actual" }
if (Test-Path -LiteralPath $extract) { Remove-Item -LiteralPath $extract -Recurse -Force }
Expand-Archive -LiteralPath $archive -DestinationPath $extract
& (Join-Path $extract 'install.ps1') -Source (Join-Path $extract 'product') -InstallRoot $InstallRoot
