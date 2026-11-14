@echo off
setlocal

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-TesmioPlugin.ps1" -NoPause %*
set "installExitCode=%ERRORLEVEL%"

if not "%installExitCode%"=="0" (
    echo.
    echo Earthworks installation failed. See the message above.
    pause
    exit /b %installExitCode%
)

echo.
echo Earthworks installation finished. Start the game through TesmioLoader.
pause
exit /b 0
