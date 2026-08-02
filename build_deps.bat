@echo off
pushd "%~dp0"
set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"

echo ===========================================
echo 1. Initializing ARM64 Environment
echo ===========================================
call "%VCVARS%" arm64 > nul

echo ===========================================
echo 2. Building Dependencies (ARM64)
echo ===========================================
msbuild ..\sigpro\VS18\zlib.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=ARM64 /p:PlatformToolset=v145
if errorlevel 1 pause
msbuild ..\sigpro\VS18\sigpro_lib.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=ARM64 /p:PlatformToolset=v145
if errorlevel 1 pause
msbuild ..\arsc\arsc_VS18\arsc_lib.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=ARM64 /p:PlatformToolset=v145
if errorlevel 1 pause

echo ===========================================
echo 3. Initializing x86 Environment
echo ===========================================
call "%VCVARS%" x86 > nul

echo ===========================================
echo 4. Building Dependencies (x86)
echo ===========================================
msbuild ..\sigpro\VS18\zlib.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=Win32 /p:PlatformToolset=v145
if errorlevel 1 pause
msbuild ..\sigpro\VS18\sigpro_lib.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=Win32 /p:PlatformToolset=v145
if errorlevel 1 pause
msbuild ..\arsc\arsc_VS18\arsc_lib.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=Win32 /p:PlatformToolset=v145
if errorlevel 1 pause

echo [SUCCESS] Dependencies Built.
pause
popd
