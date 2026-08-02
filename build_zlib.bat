@echo off
set "VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"
call "%VCVARS%" arm64 > nul
echo Compiling zlib (ARM64)...
msbuild ..\sigpro\VS18\zlib.vcxproj /p:Configuration=Release /p:Platform=ARM64 /p:PlatformToolset=v145
pause
