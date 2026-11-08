@echo off
echo This launcher stages the current Earthworks plugin and dedicated facility before opening the project-local RML.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Launch-Diagnostics.ps1" %*
set "launchExitCode=%errorlevel%"
if not "%launchExitCode%"=="0" (
    echo.
    echo Prototype launch failed with exit code %launchExitCode%. Review the error above.
    pause
)
exit /b %launchExitCode%
