[CmdletBinding()]
param(
    [string]$QtRoot = $env:AIDC_QT_ROOT,
    [string]$CompilerRoot = $env:AIDC_MINGW_ROOT
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path $PSScriptRoot).Path
if (-not $QtRoot) { $QtRoot = 'D:\Software\Qt\6.8.1\mingw_64' }
if (-not $CompilerRoot) { $CompilerRoot = 'D:\Software\Qt\Tools\mingw1310_64' }
$cmake = (Get-Command cmake -ErrorAction Stop).Source
$make = Join-Path $CompilerRoot 'bin\mingw32-make.exe'
$compiler = Join-Path $CompilerRoot 'bin\g++.exe'
$deploy = Join-Path $QtRoot 'bin\windeployqt.exe'
foreach ($required in @($make, $compiler, $deploy)) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Required build tool not found: $required" }
}

$build = Join-Path $root 'build'
$dist = Join-Path $root 'dist'
& $cmake -S $root -B $build -G 'MinGW Makefiles' `
    "-DCMAKE_MAKE_PROGRAM=$($make.Replace('\','/'))" `
    "-DCMAKE_CXX_COMPILER=$($compiler.Replace('\','/'))" `
    "-DCMAKE_PREFIX_PATH=$($QtRoot.Replace('\','/'))" `
    '-DCMAKE_BUILD_TYPE=Release'
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed: $LASTEXITCODE" }
& $cmake --build $build --parallel 4
if ($LASTEXITCODE -ne 0) { throw "Runtime build failed: $LASTEXITCODE" }

$env:PATH = "$(Join-Path $QtRoot 'bin');$(Join-Path $CompilerRoot 'bin');$env:PATH"
& ctest --test-dir $build --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Runtime contract tests failed: $LASTEXITCODE" }

if (Test-Path -LiteralPath $dist) { Remove-Item -LiteralPath $dist -Recurse -Force }
& $cmake --install $build --prefix $dist
if ($LASTEXITCODE -ne 0) { throw "Runtime install failed: $LASTEXITCODE" }
$unversioned = Join-Path $dist 'bin\aidc-sysml-parser-runtime.exe'
$versioned = Join-Path $dist 'bin\aidc-sysml-parser-runtime-0.9.0.exe'
Copy-Item -LiteralPath $unversioned -Destination $versioned -Force
& $deploy --release --no-translations --no-system-d3d-compiler --no-opengl-sw $versioned
if ($LASTEXITCODE -ne 0) { throw "Qt runtime deployment failed: $LASTEXITCODE" }
Write-Host "Standalone SysML runtime created: $dist"
