@echo off
REM build_and_test.bat - Automated MSVC Build Verification Script for EMAV

echo ===================================================
echo   EMAV MSVC Automated Build Verification
echo ===================================================
echo.

REM 1. Locate Visual Studio vcvarsall.bat
set "VCVARS="
if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
)

if "%VCVARS%"=="" (
    echo [ERROR] Visual Studio vcvarsall.bat not found!
    exit /b 1
)

echo [1/3] Initializing MSVC Developer Environment...
call "%VCVARS%" x86 > nul
if errorlevel 1 (
    echo [ERROR] Failed to initialize MSVC environment.
    exit /b 1
)

echo [2/3] Building EMAV Solution...
msbuild VS16\emav.vcxproj /p:Configuration=Release /p:Platform=Win32 /nologo /verbosity:minimal
if errorlevel 1 (
    echo.
    echo [FAIL] MSBuild compilation failed!
    exit /b 1
)

echo.
echo [3/3] Verifying Output Executable...
if exist "VS16\Release\aemav.exe" (
    echo ===================================================
    echo   [SUCCESS] EMAV Build Succeeded!
    echo   Executable: VS16\Release\aemav.exe
    echo ===================================================
    exit /b 0
) else (
    echo [FAIL] Executable VS16\Release\aemav.exe was not produced.
    exit /b 1
)
