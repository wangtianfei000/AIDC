$ErrorActionPreference = "Stop"
$modelicaBrowser = Split-Path -Parent $PSScriptRoot
$repoRoot = Split-Path -Parent (Split-Path -Parent $modelicaBrowser)
$native = Join-Path $modelicaBrowser "libraries/AIDCRobotics/native/modelica-native-geometry"
$resources = Join-Path $modelicaBrowser "libraries/AIDCRobotics/modelica/AIDCRobotics/Resources"
$build = Join-Path $modelicaBrowser "results/native-geometry-build"
$gcc = "D:/Software/Qt/Tools/mingw1310_64/bin/gcc.exe"
$ar = "D:/Software/Qt/Tools/mingw1310_64/bin/ar.exe"
New-Item -ItemType Directory -Force -Path (Join-Path $resources "Include"),(Join-Path $resources "Library/win64"),$build | Out-Null
Copy-Item -Force (Join-Path $native "include/aidc_geometry.h") (Join-Path $resources "Include/aidc_geometry.h")
& $gcc -O2 -std=c99 -shared -I (Join-Path $native "include") (Join-Path $native "src/aidc_geometry.c") -o (Join-Path $resources "Library/win64/aidc_geometry.dll") "-Wl,--out-implib,$((Join-Path $resources 'Library/win64/libaidc_geometry.dll.a').Replace('\','/'))"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $gcc -O2 -std=c99 -I (Join-Path $native "include") -c (Join-Path $native "src/aidc_geometry.c") -o (Join-Path $build "aidc_geometry.o")
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $ar rcs (Join-Path $resources "Library/win64/libaidc_geometry.a") (Join-Path $build "aidc_geometry.o")
exit $LASTEXITCODE
