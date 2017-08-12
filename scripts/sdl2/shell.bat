@echo off

REM set VCDIR="C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC"
REM call %VCDIR%\vcvarsall.bat x64
REM
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools"\VsDevCmd.bat -arch=amd64 -host_arch=amd64

set PATH=%~dp0;%PATH%
