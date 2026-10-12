[CmdletBinding()]
param(
  [int]$Model3DPort = 4176,
  [int]$ModelicaPort = 4186
)
$ErrorActionPreference = 'Stop'
$model3d = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$aidc = (Resolve-Path (Join-Path $model3d '..')).Path
$modelica = Join-Path $aidc 'modelica-browser'
$fixture = Join-Path $model3d 'test-artifacts\modelica-export-single-joint'
$assembly = Join-Path $fixture 'release\aidc.standard.single-joint-p0-r1.aidc-assembly'
$generatedProject = Join-Path $modelica 'generated\browser-e2e-single-joint'
$logDir = Join-Path $fixture '.test-logs'
$model3dProcess = $null
$modelicaProcess = $null

function Wait-Http([string]$Url) {
  for ($attempt = 0; $attempt -lt 80; $attempt++) {
    try { Invoke-WebRequest -UseBasicParsing -Uri $Url -TimeoutSec 2 | Out-Null; return }
    catch { Start-Sleep -Milliseconds 250 }
  }
  throw "Service did not become ready: $Url"
}

try {
  python -m unittest (Join-Path $model3d 'tools\freecad-publisher\tests\test_export_modelica_assembly.py')
  if ($LASTEXITCODE -ne 0) { throw 'Modelica assembly exporter unit tests failed' }
  python (Join-Path $model3d 'tools\freecad-publisher\tests\build_modelica_export_fixture.py') | Out-Host
  if ($LASTEXITCODE -ne 0) { throw 'Failed to build the test assembly' }
  New-Item -ItemType Directory -Force -Path $logDir | Out-Null
  if (Test-Path -LiteralPath $assembly) { Remove-Item -LiteralPath $assembly -Recurse -Force }
  if (Test-Path -LiteralPath $generatedProject) { Remove-Item -LiteralPath $generatedProject -Recurse -Force }

  $env:AIDC_MODEL3D_PROJECT_DIR = $fixture
  $env:AIDC_MODEL3D_ROBOT_IR = Join-Path $fixture 'robot-ir'
  $env:AIDC_MODELICA_ASSEMBLY_OUTPUT = $assembly
  $model3dProcess = Start-Process -FilePath 'npm.cmd' -ArgumentList @('exec','vite','--','--host','127.0.0.1','--port',"$Model3DPort",'--strictPort') -WorkingDirectory (Join-Path $model3d 'apps\browser') -WindowStyle Hidden -RedirectStandardOutput (Join-Path $logDir 'model3d.stdout.log') -RedirectStandardError (Join-Path $logDir 'model3d.stderr.log') -PassThru
  Wait-Http "http://127.0.0.1:$Model3DPort/"
  $export = Invoke-RestMethod -Method Post -Uri "http://127.0.0.1:$Model3DPort/api/model-projects/aidc.standard.single-joint/modelica-assembly"
  if (-not $export.passed -or -not $export.valid) { throw 'Model3D browser export API did not return a valid assembly package' }

  python (Join-Path $modelica 'tools\aidc_modelica.py') validate $assembly | Out-Host
  if ($LASTEXITCODE -ne 0) { throw 'Modelica assembly package validation failed' }

  $seed = Join-Path $aidc 'samples\modelica-t0-single-joint'
  $modelicaProcess = Start-Process -FilePath 'powershell.exe' -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',(Join-Path $modelica 'scripts\start-project-server.ps1'),'-Project',$seed,'-Port',"$ModelicaPort") -WindowStyle Hidden -RedirectStandardOutput (Join-Path $logDir 'modelica.stdout.log') -RedirectStandardError (Join-Path $logDir 'modelica.stderr.log') -PassThru
  Wait-Http "http://127.0.0.1:$ModelicaPort/api/v1/project"
  $request = @{ source = $assembly; output = $generatedProject; profile = 'auto'; migrate = $false; force = $true; stlUnit = 'm'; density = 1000; rootType = 'fixed' } | ConvertTo-Json
  $conversion = Invoke-RestMethod -Method Post -Uri "http://127.0.0.1:$ModelicaPort/api/v1/model-conversion" -ContentType 'application/json' -Body $request
  if (-not $conversion.project) { throw 'Modelica browser conversion API did not return a project' }

  & (Join-Path $modelica 'scripts\run-project.ps1') -Project $generatedProject
  if ($LASTEXITCODE -ne 0) { throw 'OMC simulation of the generated project failed' }
  [ordered]@{
    passed = $true
    model3dExport = $export
    modelicaProject = $conversion.project
    assembly = $assembly
    generatedProject = $generatedProject
  } | ConvertTo-Json -Depth 12 | Write-Host
}
finally {
  if ($modelicaProcess -and -not $modelicaProcess.HasExited) { Stop-Process -Id $modelicaProcess.Id -Force }
  if ($model3dProcess -and -not $model3dProcess.HasExited) { Stop-Process -Id $model3dProcess.Id -Force }
}
