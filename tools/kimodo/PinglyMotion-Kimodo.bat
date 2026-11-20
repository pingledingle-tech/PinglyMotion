@echo off
rem PinglyMotion helper for NVIDIA Kimodo (text-to-motion). Double-click to start.
rem From a command prompt: PinglyMotion-Kimodo.bat make "wave with the right hand" 4s
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0kimodo_setup.ps1" %*
set CODE=%ERRORLEVEL%
echo.
if "%~1"=="" pause
exit /b %CODE%
