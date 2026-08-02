@echo off
pushd "%~dp0"

echo ===================================================
echo   EMAV Inno Setup Compiler
echo ===================================================

REM Search for Inno Setup Compiler
set "ISCC="
if exist "..\setup\iscc.exe" set "ISCC=..\setup\iscc.exe"
if exist "C:\Program Files (x86)\Inno Setup 5\ISCC.exe" set "ISCC=C:\Program Files (x86)\Inno Setup 5\ISCC.exe"
if exist "C:\Program Files\Inno Setup 6\ISCC.exe" set "ISCC=C:\Program Files\Inno Setup 6\ISCC.exe"
if exist "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" set "ISCC=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"

if "%ISCC%"=="" (
    echo [ERROR] Inno Setup Compiler ISCC.exe was not found!
    echo.
    echo It looks like Inno Setup is not installed on this machine.
    echo Please download and install it from:
    echo https://jrsoftware.org/isdl.php
    echo.
    pause
    exit /b 1
)

echo.
echo [1/1] Compiling Installer using ISCC...
"%ISCC%" VS18\emav_installer.iss

if errorlevel 1 (
    echo.
    echo [FAIL] Failed to compile the installer.
    pause
    exit /b 1
)

echo.
echo ===================================================
echo   [SUCCESS] Installer Generated!
echo   Location: VS18\Output\EMAV_Setup.exe
echo ===================================================
pause
popd
