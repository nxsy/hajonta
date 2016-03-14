@echo off

set BUILDDIR="%~dp0\..\..\build"

IF NOT EXIST %BUILDDIR% mkdir %BUILDDIR%
IF NOT EXIST %BUILDDIR%\generated mkdir %BUILDDIR%\generated
pushd %BUILDDIR%

set INCLUDES=-I..\source -Igenerated -Zi -I..\source\hajonta\thirdparty -I..\thirdparty\stb -I..\thirdparty\imgui -I..\thirdparty\tinyobjloader

set GAME_WARNINGS=/Wall /wd4820 /wd4668 /wd4996 /wd4100 /wd4514 /wd4191 /wd4201 /wd4505 /wd4710
set TOOL_WARNINGS=/W4 -D_CRT_SECURE_NO_WARNINGS=1
set COMMON_CPPFLAGS=%includes% /FC /nologo /EHsc
set CPPFLAGS=%COMMON_CPPFLAGS% %GAME_WARNINGS%
set TOOL_CPPFLAGS=%COMMON_CPPFLAGS% %TOOL_WARNINGS%

cl %CPPFLAGS% /Zi ..\source\hajonta\bootstrap\program.cpp /link /incremental:no User32.lib /SUBSYSTEM:CONSOLE
REM .\program.exe ..\source hajonta\programs a
REM .\program.exe ..\source hajonta\programs debug_font
REM .\program.exe ..\source hajonta\programs b
.\program.exe ..\source hajonta\programs ui2d
REM .\program.exe ..\source hajonta\programs skybox
REM .\program.exe ..\source hajonta\programs c
.\program.exe ..\source hajonta\programs imgui
cl %CPPFLAGS% /Zi ..\source\hajonta\bootstrap\unit.cpp /link /incremental:no User32.lib /SUBSYSTEM:CONSOLE
.\unit.exe
del *.pdb > NUL 2> NUL
REM cl %CPPFLAGS% /Zi ..\source\hajonta\bootstrap\win32_msvc.cpp /link /incremental:no -PDB:win32_msvc.pdb User32.lib

echo > game2.dll.lock
cl %CPPFLAGS% -DHAJONTA_DEBUG=1 /Zi ..\source\hajonta\game2.cpp -LD /link /incremental:no -PDB:game-%random%.pdb -EXPORT:game_update_and_render Opengl32.lib
del game2.dll.lock
echo > renderer.dll.lock
cl %CPPFLAGS% -DHAJONTA_DEBUG=1 /Zi ..\source\hajonta\renderer\opengl.cpp -LD /link /incremental:no -PDB:renderer-%random%.pdb -EXPORT:renderer_setup -EXPORT:renderer_render Opengl32.lib
del renderer.dll.lock
cl %CPPFLAGS% -DHAJONTA_LIBRARY_NAME=game2.dll -DHAJONTA_RENDERER_LIBRARY_NAME=opengl.dll -DHAJONTA_DEBUG=1 ..\source\hajonta\platform\win32.cpp /link /incremental:no User32.lib Gdi32.lib Opengl32.lib Xaudio2.lib Ole32.lib Shlwapi.lib

REM echo > editor.dll.lock
REM cl %CPPFLAGS% -DHAJONTA_DEBUG=1 /Zi ..\source\hajonta\utils\editor.cpp -LD /link /incremental:no -PDB:editor-%random%.pdb -EXPORT:game_update_and_render Opengl32.lib
REM del editor.dll.lock
REM copy ..\source\hajonta\platform\win32.cpp generated\win32_editor.cpp
REM cl %CPPFLAGS% -DHAJONTA_LIBRARY_NAME=editor.dll -DHAJONTA_DEBUG=1 generated\win32_editor.cpp /link /incremental:no User32.lib Gdi32.lib Opengl32.lib Xaudio2.lib Ole32.lib Shlwapi.lib

REM echo > ddsviewer.dll.lock
REM cl %CPPFLAGS% -DHAJONTA_DEBUG=1 /Zi ..\source\hajonta\utils\ddsviewer.cpp -LD /link /incremental:no -PDB:ddsviewer-%random%.pdb -EXPORT:game_update_and_render Opengl32.lib
REM del ddsviewer.dll.lock
REM copy ..\source\hajonta\platform\win32.cpp generated\win32_ddsviewer.cpp
REM cl %CPPFLAGS% -DHAJONTA_LIBRARY_NAME=ddsviewer.dll -DHAJONTA_DEBUG=1 generated\win32_ddsviewer.cpp /link /incremental:no User32.lib Gdi32.lib Opengl32.lib Xaudio2.lib Ole32.lib Shlwapi.lib

REM cl %CPPFLAGS% -DHAJONTA_DEBUG=1 /Zi ..\source\hajonta\utils\bump_to_normal.cpp /link /incremental:no User32.lib Gdi32.lib Opengl32.lib Xaudio2.lib Ole32.lib Shlwapi.lib
cl %TOOL_CPPFLAGS% -DHAJONTA_DEBUG=1 /Zi ..\source\hajonta\utils\obj_to_mesh_v1.cpp /link /incremental:no User32.lib
echo "Build Successful"

popd
