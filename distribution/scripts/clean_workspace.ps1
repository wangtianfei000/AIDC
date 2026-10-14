param(
    [switch]$Execute,
    [switch]$IncludeLegacyCopies,
    [switch]$RuntimeBuildsOnly
)

$ErrorActionPreference = 'Stop'
$Root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..')).TrimEnd('\')

function Assert-WorkspacePath([string]$Path) {
    $absolute = [System.IO.Path]::GetFullPath($Path)
    if (-not $absolute.StartsWith($Root + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean outside the workspace: $absolute"
    }
    return $absolute
}

function Get-ItemBytes([string]$Path) {
    $item = Get-Item -LiteralPath $Path -Force
    if (-not $item.PSIsContainer) { return [int64]$item.Length }
    $sum = Get-ChildItem -LiteralPath $Path -File -Recurse -Force -ErrorAction SilentlyContinue |
        Measure-Object Length -Sum
    return [int64]$sum.Sum
}

# Only include content recoverable from source, lock files, or build scripts, plus obsolete upstream copies.
# sysml-parser-runtime/standard-library/2026-08 is retained because the runtime packages it.
$relativeTargets = @(
    '.codex-runtime',
    '.pytest_cache',
    '__pycache__',
    'build',
    'dist',
    'distribution\out',
    'node_modules',
    'aidc.spec',
    'aidc-mcp.spec',
    'aidc-model3d-export.spec',
    'aidc-modelica-server.spec',
    'aidc-modelica-tools.spec',
    'model3d-viewer\apps\browser\dist',
    'model3d-viewer\apps\hardware-test\dist',
    'model3d-viewer\test-artifacts',
    'modelica-browser\apps\web\dist',
    'modelica-browser\dist',
    'modelica-browser\generated',
    'modelica-browser\libraries\AIDCRobotics\modelica\AIDCRobotics\Resources\Library',
    'modelica-browser\libraries\AIDCRobotics\results',
    'modelica-browser\results',
    'modelica-browser\test-artifacts',
    'samples\jingang-10axis-system-design\model3d\generated',
    'samples\jingang-10axis-system-design\model3d\release\jingang-native10-p4.robot3d.zip',
    'samples\modelica-p2-manual-single-joint\results',
    'samples\modelica-t0-single-joint\results',
    'samples\modelica-t1-dc-motor\results',
    'samples\modelica-t2-single-joint\results',
    'sysml-browser\dist',
    'sysml-browser\results',
    'sysml-parser-runtime\dist',
    'sysml-parser-runtime\build',
    'sysml-parser-runtime\build-p1',
    'sysml-parser-runtime\build-p1-full',
    'sysml-parser-runtime\build-p2',
    'sysml-parser-runtime\build-p2-external',
    'sysml-parser-runtime\results'
)

if ($RuntimeBuildsOnly) {
    $relativeTargets = @(
        'sysml-parser-runtime\dist',
        'sysml-parser-runtime\build',
        'sysml-parser-runtime\build-p1',
        'sysml-parser-runtime\build-p1-full',
        'sysml-parser-runtime\build-p2',
        'sysml-parser-runtime\build-p2-external'
    )
}

if ($IncludeLegacyCopies -and -not $RuntimeBuildsOnly) {
    $relativeTargets += @('FreeCAD', 'FreeCAD-downloads', 'FreeCAD-runtime', 'freecad-mcp')
}

$targets = [System.Collections.Generic.List[object]]::new()
foreach ($relative in $relativeTargets) {
    $absolute = Assert-WorkspacePath (Join-Path $Root $relative)
    if (Test-Path -LiteralPath $absolute) {
        $targets.Add([pscustomobject]@{
            RelativePath = $relative
            AbsolutePath = $absolute
            Bytes = Get-ItemBytes $absolute
        })
    }
}

# Workspaces create nested dependency directories and Python creates distributed caches.
$generatedDirectoryNames = if ($RuntimeBuildsOnly) {
    @()
} else {
    @('node_modules', '__pycache__', '.pytest_cache', '.mypy_cache', '.ruff_cache')
}
$preservedRoots = @(
    (Join-Path $Root 'sysml-parser-runtime\standard-library\2026-08')
)
$dynamicDirectories = Get-ChildItem -LiteralPath $Root -Directory -Recurse -Force -ErrorAction SilentlyContinue |
    Where-Object {
        $candidate = $_.FullName
        $directoryName = $_.Name
        $insidePreservedRoot = @($preservedRoots | Where-Object {
            $candidate -eq $_ -or $candidate.StartsWith($_ + '\', [System.StringComparison]::OrdinalIgnoreCase)
        }).Count -gt 0
        $directoryName -in $generatedDirectoryNames -and -not $insidePreservedRoot
    } |
    Sort-Object { $_.FullName.Length }

foreach ($directory in $dynamicDirectories) {
    $absolute = Assert-WorkspacePath $directory.FullName
    $alreadyCovered = $false
    foreach ($target in $targets) {
        if ($absolute -eq $target.AbsolutePath -or
            $absolute.StartsWith($target.AbsolutePath + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
            $alreadyCovered = $true
            break
        }
    }
    if (-not $alreadyCovered) {
        $targets.Add([pscustomobject]@{
            RelativePath = $absolute.Substring($Root.Length + 1)
            AbsolutePath = $absolute
            Bytes = Get-ItemBytes $absolute
        })
    }
}

$generatedFiles = if ($RuntimeBuildsOnly) {
    @()
} else {
    @(Get-ChildItem -LiteralPath $Root -File -Recurse -Force -ErrorAction SilentlyContinue |
        Where-Object {
            $relativeFile = $_.FullName.Substring($Root.Length + 1)
            $_.Name -like '*.tsbuildinfo' -or
            $_.Name -like '*.FCBak' -or
            $_.Name -like 'tmp*.mos' -or
            $relativeFile -in @(
                'sysml-browser\preview.err.log',
                'sysml-browser\preview.out.log',
                'sysml-browser\vite.config.js',
                'sysml-browser\vite.config.d.ts',
                'sysml-parser-runtime\test-p2r1.txt'
            ) -or
            ($IncludeLegacyCopies -and $_.DirectoryName -eq $Root -and $_.Extension -eq '.lnk')
        })
}

foreach ($file in $generatedFiles) {
    $absolute = Assert-WorkspacePath $file.FullName
    $alreadyCovered = $false
    foreach ($target in $targets) {
        if ($absolute.StartsWith($target.AbsolutePath + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
            $alreadyCovered = $true
            break
        }
    }
    if (-not $alreadyCovered) {
        $targets.Add([pscustomobject]@{
            RelativePath = $absolute.Substring($Root.Length + 1)
            AbsolutePath = $absolute
            Bytes = [int64]$file.Length
        })
    }
}

$targets = @($targets | Sort-Object AbsolutePath -Unique)
$totalBytes = [int64](($targets | Measure-Object Bytes -Sum).Sum)

Write-Host "AIDC workspace: $Root"
Write-Host ("Targets: {0}; estimated space: {1:N2} GiB" -f $targets.Count, ($totalBytes / 1GB))
$targets | Sort-Object Bytes -Descending |
    Select-Object @{n='MiB';e={[math]::Round($_.Bytes / 1MB, 1)}}, RelativePath |
    Format-Table -AutoSize

if (-not $Execute) {
    Write-Host ''
    Write-Host 'Preview only. Run: npm run clean'
    exit 0
}

foreach ($target in ($targets | Sort-Object { $_.AbsolutePath.Length } -Descending)) {
    $absolute = Assert-WorkspacePath $target.AbsolutePath
    if (Test-Path -LiteralPath $absolute) {
        Remove-Item -LiteralPath $absolute -Recurse -Force
    }
}

$remaining = @($targets | Where-Object { Test-Path -LiteralPath $_.AbsolutePath })
if ($remaining.Count -gt 0) {
    throw "$($remaining.Count) cleanup targets remain."
}

Write-Host ("Cleanup complete; released about {0:N2} GiB." -f ($totalBytes / 1GB))
