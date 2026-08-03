@echo off
pushd "%~dp0"

REM Locate Visual Studio vcvarsall.bat
set "VCVARS="
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
    set "TOOLSET=v143"
) else if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"
    set "TOOLSET=v145"
)

echo ===================================================
echo   EMAV Unified Build Script (x86 + ARM64)
echo ===================================================

echo.
echo [1/4] Initializing ARM64 Environment...
call "%VCVARS%" arm64 > nul

echo [2/4] Building EMAV Solution (ARM64)...
msbuild VS18\av.sln /p:Configuration=Release /p:Platform=ARM64 /p:PlatformToolset=%TOOLSET% /v:m
if errorlevel 1 (
    echo [ERROR] ARM64 build failed!
    pause
    exit /b 1
)

echo.
echo [3/4] Initializing x86 Environment...
call "%VCVARS%" x86 > nul

echo [4/4] Building EMAV Solution (x86)...
msbuild VS18\av.sln /p:Configuration=Release /p:Platform=Win32 /p:PlatformToolset=%TOOLSET% /v:m
if errorlevel 1 (
    echo [ERROR] x86 build failed!
    pause
    exit /b 1
)

echo.

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
    exit /b 0
)

echo.
echo [1/1] Compiling Installer using ISCC...
"%ISCC%" VS18\emav_installer.iss

if errorlevel 1 (
    echo.
    echo [FAIL] Failed to compile the installer.
    exit /b 0
)

echo.
echo ===================================================
echo   [SUCCESS] Installer Generated!
echo   Location: VS18\Output\EMAV_Setup.exe
echo ===================================================
pause
popd
