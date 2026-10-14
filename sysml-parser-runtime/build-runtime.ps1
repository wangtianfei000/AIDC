[CmdletBinding()]
param(
    [string]$QtRoot = 'D:\Software\Qt\6.8.1\mingw_64',
    [string]$CompilerRoot = 'D:\Software\Qt\Tools\mingw1310_64',
    [string]$CMake = 'D:\Software\Qt\Tools\CMake_64\bin\cmake.exe',
    [switch]$SkipTests
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path $PSScriptRoot).Path
$build = Join-Path $root 'build'
$dist = Join-Path $root 'dist'
$compiler = Join-Path $CompilerRoot 'bin\g++.exe'
$make = Join-Path $CompilerRoot 'bin\mingw32-make.exe'
$deployQt = Join-Path $QtRoot 'bin\windeployqt.exe'
$cmakeQtRoot = $QtRoot.Replace('\', '/')
$cmakeCompiler = $compiler.Replace('\', '/')
$cmakeMake = $make.Replace('\', '/')

foreach ($required in @($CMake, $compiler, $make, $deployQt)) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Required build input was not found: $required" }
}
foreach ($path in @($build, $dist)) {
    if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Recurse -Force }
}

$configureArguments = @(
    '-S'
    $root
    '-B'
    $build
    '-G'
    'MinGW Makefiles'
    "-DCMAKE_PREFIX_PATH=$cmakeQtRoot"
    "-DCMAKE_CXX_COMPILER=$cmakeCompiler"
    "-DCMAKE_MAKE_PROGRAM=$cmakeMake"
    '-DCMAKE_BUILD_TYPE=Release'
)
& $CMake @configureArguments
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed: $LASTEXITCODE" }

& $CMake --build $build --parallel 4
if ($LASTEXITCODE -ne 0) { throw "Runtime build failed: $LASTEXITCODE" }
if (-not $SkipTests) {
    $env:PATH = "$(Join-Path $QtRoot 'bin');$(Join-Path $CompilerRoot 'bin');$env:PATH"
    & $CMake --build $build --target test
    if ($LASTEXITCODE -ne 0) { throw "Runtime contract tests failed: $LASTEXITCODE" }
}

& $CMake --install $build --prefix $dist
if ($LASTEXITCODE -ne 0) { throw "Runtime install failed: $LASTEXITCODE" }
$runtime = Join-Path $dist 'bin\aidc-sysml-parser-runtime.exe'
Copy-Item -LiteralPath $runtime -Destination (Join-Path $dist 'bin\aidc-sysml-parser-runtime-1.0.0.exe') -Force
& $deployQt --release --no-translations --no-system-d3d-compiler --no-opengl-sw $runtime
if ($LASTEXITCODE -ne 0) { throw "Qt deployment failed: $LASTEXITCODE" }

Write-Host "C++ ANTLR + Qt SysML runtime created: $dist"
