@echo off

call build_base.bat

cl %CPPFLAGS% /Zi ..\source\hajonta\bootstrap\program.cpp /link /incremental:no User32.lib /SUBSYSTEM:CONSOLE
.\program.exe ..\source hajonta\programs ui2d
.\program.exe ..\source hajonta\programs imgui
.\program.exe ..\source hajonta\programs sky
.\program.exe ..\source hajonta\programs phong_no_normal_map
.\program.exe ..\source hajonta\programs texarray_1
.\program.exe ..\source hajonta\programs texarray_1_vsm
.\program.exe ..\source hajonta\programs variance_shadow_map
.\program.exe ..\source hajonta\programs\filters filter_gaussian_7x1
cl %CPPFLAGS% /Zi ..\source\hajonta\bootstrap\unit.cpp /link /incremental:no User32.lib /SUBSYSTEM:CONSOLE
.\unit.exe
del *.pdb > NUL 2> NUL

cl %CPPFLAGS% /Zi ..\source\hajonta\bootstrap\bootstrap.cpp /link /incremental:no User32.lib /SUBSYSTEM:CONSOLE
.\bootstrap.exe

echo > game2.dll.lock
cl %CPPFLAGS% -DHAJONTA_DEBUG=1 /Zi ..\source\hajonta\game2.cpp -LD /link /incremental:no -PDB:game2-%random%.pdb -EXPORT:game_update_and_render -EXPORT:game_debug_frame_end Opengl32.lib
del game2.dll.lock

echo > renderer.dll.lock
cl %CPPFLAGS% -DHAJONTA_DEBUG=1 /Zi ..\source\hajonta\renderer\opengl.cpp -LD /link /incremental:no -PDB:renderer-%random%.pdb -EXPORT:renderer_setup -EXPORT:renderer_render Opengl32.lib
del renderer.dll.lock

cl /MD %CPPFLAGS% -DHAJONTA_LIBRARY_NAME=game2.dll -DHAJONTA_RENDERER_LIBRARY_NAME=opengl.dll -DHAJONTA_DEBUG=1 ..\source\hajonta\platform\sdl2.cpp /link /incremental:no User32.lib Gdi32.lib Opengl32.lib Xaudio2.lib Ole32.lib Shlwapi.lib ..\thirdparty\sdl2\lib\x64\SDL2.lib
xcopy /Q /D ..\thirdparty\sdl2\lib\x64\SDL2.dll . >NULL || exit /b
popd
