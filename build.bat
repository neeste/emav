@echo off
pushd "%~dp0"
REM build.bat - Automated MSVC Build Script for EMAV

echo ===================================================
echo   EMAV MSVC Local Build Script
echo ===================================================
echo.

REM Locate Visual Studio vcvarsall.bat
set "VCVARS="
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
)

if "%VCVARS%"=="" (
    echo [ERROR] Visual Studio vcvarsall.bat not found!
    echo Please ensure Visual Studio with C++ Desktop Development is installed.
    pause
    exit /b 1
)

echo [1/3] Initializing MSVC Developer Environment...
call "%VCVARS%" x86 > nul
if errorlevel 1 (
    echo [ERROR] Failed to initialize MSVC environment.
    pause
    exit /b 1
)

echo [2/3] Building EMAV Solution...
msbuild VS16\av.sln /p:Configuration=Release /p:Platform=Win32 /nologo /verbosity:minimal
if errorlevel 1 (
    echo.
    echo [FAIL] MSBuild compilation failed!
    pause
    exit /b 1
)

echo.
echo [3/3] Verifying Output Executable...
if exist "VS16\Release\aemav.exe" (
    echo ===================================================
    echo   [SUCCESS] EMAV Build Succeeded!
    echo   Executable: VS16\Release\aemav.exe
    echo ===================================================
) else (
    echo [FAIL] Executable VS16\Release\aemav.exe was not produced.
)

pause
popd
exit /b 0
