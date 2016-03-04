@echo off

set BUILDDIR="%~dp0\..\..\build"

IF NOT EXIST %BUILDDIR% mkdir %BUILDDIR%
IF NOT EXIST %BUILDDIR%\generated mkdir %BUILDDIR%\generated
pushd %BUILDDIR%

set INCLUDES=-I..\source -Igenerated -Zi -I..\source\hajonta\thirdparty -I..\thirdparty\sdl2\include -I..\thirdparty\stb -I..\thirdparty\imgui
set CPPFLAGS=%includes% /FC /nologo /Wall /wd4820 /wd4668 /wd4996 /wd4100 /wd4514 /wd4191 /wd4201 /wd4505 /wd4710 /EHsc

cl %CPPFLAGS% /Zi ..\source\hajonta\bootstrap\program.cpp /link /incremental:no User32.lib /SUBSYSTEM:CONSOLE
.\program.exe ..\source hajonta\programs ui2d
.\program.exe ..\source hajonta\programs imgui
cl %CPPFLAGS% /Zi ..\source\hajonta\bootstrap\unit.cpp /link /incremental:no User32.lib /SUBSYSTEM:CONSOLE
.\unit.exe
del *.pdb > NUL 2> NUL

echo > game2.dll.lock
cl %CPPFLAGS% -DHAJONTA_DEBUG=1 /Zi ..\source\hajonta\game2.cpp -LD /link /incremental:no -PDB:game2-%random%.pdb -EXPORT:game_update_and_render Opengl32.lib
del game2.dll.lock

echo > renderer.dll.lock
cl %CPPFLAGS% -DHAJONTA_DEBUG=1 /Zi ..\source\hajonta\renderer\opengl.cpp -LD /link /incremental:no -PDB:renderer-%random%.pdb -EXPORT:renderer_setup -EXPORT:renderer_render Opengl32.lib
del renderer.dll.lock

cl /MD %CPPFLAGS% -DHAJONTA_LIBRARY_NAME=game2.dll -DHAJONTA_RENDERER_LIBRARY_NAME=opengl.dll -DHAJONTA_DEBUG=1 ..\source\hajonta\platform\sdl2.cpp /link /incremental:no User32.lib Gdi32.lib Opengl32.lib Xaudio2.lib Ole32.lib Shlwapi.lib ..\thirdparty\sdl2\lib\x64\SDL2.lib

copy ..\thirdparty\sdl2\lib\x64\SDL2.dll .
popd
