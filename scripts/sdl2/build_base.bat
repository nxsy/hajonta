@echo off

set BUILDDIR="%~dp0\..\..\build"
set "ASSETPATH=%~dp0\..\..\..\hajonta-assets"
set "ASSETPATH=%ASSETPATH:\=\\%"

IF NOT EXIST %BUILDDIR% mkdir %BUILDDIR%
IF NOT EXIST %BUILDDIR%\generated mkdir %BUILDDIR%\generated
pushd %BUILDDIR%

set INCLUDES=-I..\source -Igenerated -Zi -I..\source\hajonta\thirdparty -I..\thirdparty\sdl2\include -I..\thirdparty\stb -I..\thirdparty\imgui -I..\thirdparty\tinyobjloader -I..\thirdparty\par

set GAME_WARNINGS=/Wall /wd4820 /wd4668 /wd4996 /wd4100 /wd4514 /wd4191 /wd4201 /wd4505 /wd4710 /wd4625 /wd5026 /wd4626 /wd5027 /wd4711 /wd4429
set TOOL_WARNINGS=/W4 -D_CRT_SECURE_NO_WARNINGS=1 /wd4201
set COMMON_CPPFLAGS=%includes% /FC /nologo /Od
set CPPFLAGS=%COMMON_CPPFLAGS% %GAME_WARNINGS% /EHsc
set TOOL_CPPFLAGS=%COMMON_CPPFLAGS% %TOOL_WARNINGS% -DHAJONTA_DEBUG=1 -D_HAS_EXCEPTIONS=0
set TOOL_LINK=/link /incremental:no User32.lib
set TOOL_PATH=..\source\hajonta\utils
