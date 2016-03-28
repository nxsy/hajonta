@echo off

set BUILDDIR="%~dp0\..\..\build"

IF NOT EXIST %BUILDDIR% mkdir %BUILDDIR%
IF NOT EXIST %BUILDDIR%\generated mkdir %BUILDDIR%\generated
pushd %BUILDDIR%

set INCLUDES=-I..\source -Igenerated -Zi -I..\source\hajonta\thirdparty -I..\thirdparty\sdl2\include -I..\thirdparty\stb -I..\thirdparty\imgui -I..\thirdparty\tinyobjloader

set GAME_WARNINGS=/Wall /wd4820 /wd4668 /wd4996 /wd4100 /wd4514 /wd4191 /wd4201 /wd4505 /wd4710
set TOOL_WARNINGS=/W4 -D_CRT_SECURE_NO_WARNINGS=1 /wd4201
set COMMON_CPPFLAGS=%includes% /FC /nologo /EHsc
set CPPFLAGS=%COMMON_CPPFLAGS% %GAME_WARNINGS%
set TOOL_CPPFLAGS=%COMMON_CPPFLAGS% %TOOL_WARNINGS%

cl %CPPFLAGS% /Zi ..\source\hajonta\bootstrap\program.cpp /link /incremental:no User32.lib /SUBSYSTEM:CONSOLE
.\program.exe ..\source hajonta\programs ui2d
.\program.exe ..\source hajonta\programs imgui
.\program.exe ..\source hajonta\programs phong_no_normal_map
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

cl %TOOL_CPPFLAGS% -DHAJONTA_DEBUG=1 /Zi ..\source\hajonta\utils\obj_to_mesh_v1.cpp /link /incremental:no User32.lib
xcopy /Q /D ..\thirdparty\sdl2\lib\x64\SDL2.dll . >NULL || exit /b
IF EXIST ..\..\thirdparty\assimp\lib\RelWithDebInfo (
    xcopy /Q /D ..\..\thirdparty\assimp\lib\RelWithDebInfo\* %BUILDDIR%\ 
    REM >NUL || exit /b
    xcopy /Q /D ..\..\thirdparty\assimp\x64\RelWithDebInfo\* %BUILDDIR%\ 
    REM >NUL || exit /b
    IF NOT EXIST %BUILDDIR%\generated\assimp mkdir %BUILDDIR%\generated\assimp
    xcopy /Q /D /E ..\..\thirdparty\assimp\include\assimp %BUILDDIR%\generated\assimp 
    REM >NUL || exit /b
)

IF EXIST %BUILDDIR%\assimp-vc140-mt.lib (
    cl %TOOL_CPPFLAGS% -DHAJONTA_DEBUG=1 /Zi ..\source\hajonta\utils\assimp_to_mesh_v1.cpp /link /incremental:no User32.lib assimp\assimp-vc140-mt.lib
)

popd
