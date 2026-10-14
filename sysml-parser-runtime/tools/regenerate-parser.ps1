[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$AntlrJar,
    [string]$Java = 'java',
    [string]$EngineVersion,
    [switch]$AcceptChanges
)

$ErrorActionPreference = 'Stop'
$runtimeRoot = Split-Path -Parent $PSScriptRoot
$grammar = Join-Path $runtimeRoot 'engine\parser\grammar\SysML.g4'
$generated = Join-Path $runtimeRoot 'engine\parser\generated'
$expectedVersion = '4.13.2'

if (-not (Test-Path -LiteralPath $AntlrJar -PathType Leaf)) {
    throw "ANTLR Tool JAR was not found: $AntlrJar"
}
if ($AcceptChanges -and [string]::IsNullOrWhiteSpace($EngineVersion)) {
    throw '-EngineVersion x.y.z is required with -AcceptChanges.'
}
$version = (& $Java -jar $AntlrJar 2>&1 | Select-Object -First 1) -join ''
if ($version -notmatch [regex]::Escape($expectedVersion)) {
    throw "ANTLR Tool $expectedVersion is required; reported: $version"
}

$temporary = Join-Path ([System.IO.Path]::GetTempPath()) ("aidc-sysml-antlr-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temporary | Out-Null
try {
    Push-Location (Split-Path -Parent $grammar)
    try {
        & $Java -jar $AntlrJar -Dlanguage=Cpp -visitor -no-listener -o $temporary -package SysML (Split-Path -Leaf $grammar)
        if ($LASTEXITCODE -ne 0) { throw "ANTLR generation failed: $LASTEXITCODE" }
    }
    finally {
        Pop-Location
    }
    $expected = @(
        'SysML.interp', 'SysML.tokens', 'SysMLBaseVisitor.cpp', 'SysMLBaseVisitor.h',
        'SysMLLexer.cpp', 'SysMLLexer.h', 'SysMLLexer.interp', 'SysMLLexer.tokens',
        'SysMLParser.cpp', 'SysMLParser.h', 'SysMLVisitor.cpp', 'SysMLVisitor.h'
    )
    foreach ($name in $expected | Where-Object { $_ -match '\.(cpp|h)$' }) {
        $candidate = Join-Path $temporary $name
        $content = [System.IO.File]::ReadAllText($candidate)
        $content = [regex]::Replace($content, '(?m)^// Generated from .* by ANTLR ', '// Generated from engine/parser/grammar/SysML.g4 by ANTLR ')
        [System.IO.File]::WriteAllText($candidate, $content, [System.Text.UTF8Encoding]::new($false))
    }
    $actual = @(Get-ChildItem -LiteralPath $temporary -File | Select-Object -ExpandProperty Name | Sort-Object)
    if (Compare-Object ($expected | Sort-Object) $actual) {
        throw "Generated file set differs from the fixed 12-file contract."
    }
    $changed = @()
    foreach ($name in $expected) {
        $candidate = Join-Path $temporary $name
        $current = Join-Path $generated $name
        if (-not (Test-Path -LiteralPath $current) -or
            (Get-FileHash -Algorithm SHA256 $candidate).Hash -ne (Get-FileHash -Algorithm SHA256 $current).Hash) {
            $changed += $name
        }
    }
    if ($changed.Count -gt 0 -and -not $AcceptChanges) {
        throw "Generated output changed: $($changed -join ', '). Review it, then rerun with -AcceptChanges."
    }
    if ($AcceptChanges) {
        foreach ($name in $expected) {
            Copy-Item -LiteralPath (Join-Path $temporary $name) -Destination (Join-Path $generated $name) -Force
        }
        & python -X utf8 (Join-Path $PSScriptRoot 'update_engine_maintenance.py') --engine-version $EngineVersion --note 'Grammar and ANTLR generated sources updated together'
        if ($LASTEXITCODE -ne 0) { throw "Engine maintenance manifest update failed: $LASTEXITCODE" }
    }
    Write-Host "ANTLR $expectedVersion generation verified; changed files: $($changed.Count)"
}
finally {
    if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary -Recurse -Force }
}
