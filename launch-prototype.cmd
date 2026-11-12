@echo off
echo This launcher stages the Earthworks plugin and facility, then opens the installed TesmioLoader.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Launch-Tesmio.ps1" %*
set "launchExitCode=%errorlevel%"
if not "%launchExitCode%"=="0" (
    echo.
    echo Prototype launch failed with exit code %launchExitCode%. Review the error above.
    pause
)
exit /b %launchExitCode%
