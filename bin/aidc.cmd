@echo off
setlocal
set "AIDC_ROOT=%~dp0.."
where python >nul 2>nul
if errorlevel 1 goto python_missing
if "%~1"=="" goto interactive
python -X utf8 "%AIDC_ROOT%\aidc.py" %*
exit /b %ERRORLEVEL%

:interactive
python -X utf8 "%AIDC_ROOT%\aidc.py" __menu
exit /b %ERRORLEVEL%

:python_missing
echo Python was not found. Install Python 3.12 or use aidc.exe from the release package.
if "%~1"=="" pause
exit /b 4
