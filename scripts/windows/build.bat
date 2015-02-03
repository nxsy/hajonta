@echo off

set BUILDDIR="%~dp0\..\..\build"

IF NOT EXIST %BUILDDIR% mkdir %BUILDDIR%
IF NOT EXIST %BUILDDIR%\generated mkdir %BUILDDIR%\generated
pushd %BUILDDIR%

set INCLUDES=-I..\source -Igenerated
set CPPFLAGS=%includes% /FC /nologo /Wall /wd4820 /wd4668 /wd4996 /wd4200 /wd4100 /wd4514 /wd4191

cl %CPPFLAGS% /Zi ..\source\hajonta\bootstrap\program.cpp /link /incremental:no User32.lib /SUBSYSTEM:CONSOLE
.\program.exe ..\source hajonta\programs a
.\program.exe ..\source hajonta\programs debug_font
cl %CPPFLAGS% /Zi ..\source\hajonta\bootstrap\unit.cpp /link /incremental:no User32.lib /SUBSYSTEM:CONSOLE
.\unit.exe

del *.pdb > NUL 2> NUL
echo > game.dll.lock
cl %CPPFLAGS% -DHAJONTA_DEBUG=1 /Zi ..\source\hajonta\game.cpp -LD /link /incremental:no -PDB:game-%random%.pdb -EXPORT:game_update_and_render Opengl32.lib
del game.dll.lock
cl %CPPFLAGS% -DHAJONTA_DEBUG=1 ..\source\hajonta\platform\win32.cpp /link /incremental:no User32.lib Gdi32.lib Opengl32.lib Xaudio2.lib Ole32.lib

popd
