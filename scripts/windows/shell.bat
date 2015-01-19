@echo off

set VCDIR="C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC"

call %VCDIR%\vcvarsall.bat x64

set PATH=%~dp0;%PATH%
