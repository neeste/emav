@echo off
echo Cleaning up accidental files...
del /Q c:\usr\lib\arm64
del /Q c:\usr\lib\x86

echo Creating correct directories...
mkdir c:\usr\lib\arm64
mkdir c:\usr\lib\x86
mkdir c:\usr\include

echo Done!
pause
