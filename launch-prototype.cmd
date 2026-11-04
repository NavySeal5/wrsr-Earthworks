@echo off
echo This launcher stages the current Earthworks plugin and dedicated facility before opening the project-local RML.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Launch-Diagnostics.ps1"
exit /b %errorlevel%
