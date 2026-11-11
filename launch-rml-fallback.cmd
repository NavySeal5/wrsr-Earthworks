@echo off
echo This fallback stages the diagnostic profile for Republic Mod Loader.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Launch-Diagnostics.ps1" %*
set "launchExitCode=%errorlevel%"
if not "%launchExitCode%"=="0" (
    echo.
    echo RML fallback launch failed with exit code %launchExitCode%. Review the error above.
    pause
)
exit /b %launchExitCode%
