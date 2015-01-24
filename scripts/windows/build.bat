@echo off

set BUILDDIR="%~dp0\..\..\build"

IF NOT EXIST %BUILDDIR% mkdir %BUILDDIR%
pushd %BUILDDIR%

del *.pdb > NUL 2> NUL
echo > game.dll.lock
cl /FC /nologo /Zi /I..\source ..\source\hajonta\game.cpp -LD /link /incremental:no -PDB:game-%random%.pdb -EXPORT:game_update_and_render Opengl32.lib
del game.dll.lock
cl /FC /nologo /Zi /I..\source ..\source\hajonta\platform\win32.cpp /link /incremental:no User32.lib Gdi32.lib Opengl32.lib

popd
