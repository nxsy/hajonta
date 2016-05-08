@echo off

call build_base.bat

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
    cl %TOOL_CPPFLAGS% ..\source\hajonta\utils\assimp_to_mesh_v1.cpp /link /incremental:no -PDB:assimp_to_mesh_v1.pdb User32.lib assimp\assimp-vc140-mt.lib
)

cl %TOOL_CPPFLAGS% %TOOL_PATH%\obj_to_mesh_v1.cpp %TOOL_LINK%
cl %TOOL_CPPFLAGS% %TOOL_PATH%\palette_image.cpp %TOOL_LINK%

popd
