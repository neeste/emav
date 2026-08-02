@echo off
pushd "%~dp0"
echo Running ARM64 (WIN64) version of EMAV...
if exist "VS18\ARM64\Release\aemav.exe" (
    pushd "VS18\ARM64\Release"
    start "" "aemav.exe"
    popd
) else (
    echo Error: Could not find VS18\ARM64\Release\aemav.exe
    echo Please make sure you have run build_all.bat
)
popd
