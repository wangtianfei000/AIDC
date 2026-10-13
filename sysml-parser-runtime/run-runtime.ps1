param(
    [ValidateRange(0, 65535)]
    [int]$Port = 47321,
    [ValidateRange(1024, 1073741824)]
    [long]$MaximumRequestBytes = 8388608,
    [ValidateRange(1, 3600000)]
    [int]$ParseTimeoutMilliseconds = 30000
)

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$installedExecutable = Join-Path $scriptRoot 'bin\aidc-sysml-parser-runtime.exe'
$installedVersionedExecutable = Join-Path $scriptRoot 'bin\aidc-sysml-parser-runtime-0.9.0.exe'
$packagedExecutable = Join-Path $scriptRoot 'dist\bin\aidc-sysml-parser-runtime.exe'
$packagedVersionedExecutable = Join-Path $scriptRoot 'dist\bin\aidc-sysml-parser-runtime-0.9.0.exe'
$developmentExecutable = Join-Path $scriptRoot 'build\bin\aidc-sysml-parser-runtime.exe'

if (Test-Path -LiteralPath $installedVersionedExecutable) {
    $executable = $installedVersionedExecutable
} elseif (Test-Path -LiteralPath $installedExecutable) {
    $executable = $installedExecutable
} elseif (Test-Path -LiteralPath $packagedVersionedExecutable) {
    $executable = $packagedVersionedExecutable
} elseif (Test-Path -LiteralPath $packagedExecutable) {
    $executable = $packagedExecutable
} elseif (Test-Path -LiteralPath $developmentExecutable) {
    $executable = $developmentExecutable
} else {
    throw 'Parser Runtime executable was not found. Build or install the project first.'
}

& $executable `
    --port $Port `
    --max-request-bytes $MaximumRequestBytes `
    --parse-timeout-ms $ParseTimeoutMilliseconds
exit $LASTEXITCODE
