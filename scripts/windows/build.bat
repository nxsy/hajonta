@echo off

set BUILDDIR="%~dp0\..\..\build"

IF NOT EXIST %BUILDDIR% mkdir %BUILDDIR%
IF NOT EXIST %BUILDDIR%\generated mkdir %BUILDDIR%\generated
pushd %BUILDDIR%

set INCLUDES=-I..\source -Igenerated -Zi
set CPPFLAGS=%includes% /FC /nologo /Wall /wd4820 /wd4668 /wd4996 /wd4100 /wd4514 /wd4191 /wd4201 /wd4505

cl %CPPFLAGS% /Zi ..\source\hajonta\bootstrap\program.cpp /link /incremental:no User32.lib /SUBSYSTEM:CONSOLE
.\program.exe ..\source hajonta\programs a
.\program.exe ..\source hajonta\programs debug_font
.\program.exe ..\source hajonta\programs b
.\program.exe ..\source hajonta\programs ui2d
.\program.exe ..\source hajonta\programs skybox
.\program.exe ..\source hajonta\programs c
cl %CPPFLAGS% /Zi ..\source\hajonta\bootstrap\unit.cpp /link /incremental:no User32.lib /SUBSYSTEM:CONSOLE
.\unit.exe
del *.pdb > NUL 2> NUL
cl %CPPFLAGS% /Zi ..\source\hajonta\bootstrap\win32_msvc.cpp /link /incremental:no -PDB:win32_msvc.pdb User32.lib

echo > game.dll.lock
cl %CPPFLAGS% -DHAJONTA_DEBUG=1 /Zi ..\source\hajonta\game.cpp -LD /link /incremental:no -PDB:game-%random%.pdb -EXPORT:game_update_and_render Opengl32.lib
del game.dll.lock
cl %CPPFLAGS% -DHAJONTA_DEBUG=1 ..\source\hajonta\platform\win32.cpp /link /incremental:no User32.lib Gdi32.lib Opengl32.lib Xaudio2.lib Ole32.lib Shlwapi.lib

echo > editor.dll.lock
cl %CPPFLAGS% -DHAJONTA_DEBUG=1 /Zi ..\source\hajonta\utils\editor.cpp -LD /link /incremental:no -PDB:editor-%random%.pdb -EXPORT:game_update_and_render Opengl32.lib
del editor.dll.lock
copy ..\source\hajonta\platform\win32.cpp generated\win32_editor.cpp
cl %CPPFLAGS% -DHAJONTA_LIBRARY_NAME=editor.dll -DHAJONTA_DEBUG=1 generated\win32_editor.cpp /link /incremental:no User32.lib Gdi32.lib Opengl32.lib Xaudio2.lib Ole32.lib Shlwapi.lib

echo > ddsviewer.dll.lock
cl %CPPFLAGS% -DHAJONTA_DEBUG=1 /Zi ..\source\hajonta\utils\ddsviewer.cpp -LD /link /incremental:no -PDB:ddsviewer-%random%.pdb -EXPORT:game_update_and_render Opengl32.lib
del ddsviewer.dll.lock
copy ..\source\hajonta\platform\win32.cpp generated\win32_ddsviewer.cpp
cl %CPPFLAGS% -DHAJONTA_LIBRARY_NAME=ddsviewer.dll -DHAJONTA_DEBUG=1 generated\win32_ddsviewer.cpp /link /incremental:no User32.lib Gdi32.lib Opengl32.lib Xaudio2.lib Ole32.lib Shlwapi.lib

popd
