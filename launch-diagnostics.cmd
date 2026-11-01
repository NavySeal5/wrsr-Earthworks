@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Launch-Diagnostics.ps1"
exit /b %errorlevel%
