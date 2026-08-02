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
echo   [SUCCESS] Both Architectures Built!
echo ===================================================
pause
popd

