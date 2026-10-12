@echo off
setlocal
set "AIDC_ROOT=%~dp0.."
python -X utf8 "%AIDC_ROOT%\aidc.py" %*
exit /b %ERRORLEVEL%
