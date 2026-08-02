@echo off
pushd "%~dp0"
echo Running Win32 version of EMAV...
if exist "VS18\Release\aemav.exe" (
    pushd "VS18\Release"
    start "" "aemav.exe"
    popd
) else (
    echo Error: Could not find VS18\Release\aemav.exe
    echo Please make sure you have run build_all.bat
)
popd
