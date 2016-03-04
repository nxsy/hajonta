#!/bin/sh

set -e
set -u

CC=clang
WARNFLAGS="-Wall -Wno-c++11-compat-deprecated-writable-strings -Wno-unused-variable -Wno-missing-braces -Werror -ferror-limit=3 -Wno-unused-function"
DEBUG_FLAGS="-DDEBUG -g"
CPPFLAGS="-std=c++11 -DHAJONTA_DEBUG=1 -DSDL_WITH_SUBDIR=1"
INCLUDES="-Isource -Ibuild/debug/generated -Ithirdparty/stb -Ithirdparty/imgui"
CLANG="clang++"

BASEDIR=`dirname $0`/../..
cd ${BASEDIR}
mkdir -p build
mkdir -p build/debug

# code generation
${CLANG} ${CPPFLAGS} ${WARNFLAGS} -o build/debug/program source/hajonta/bootstrap/program.cpp ${DEBUG_FLAGS} ${INCLUDES}
( cd build/debug && ./program ../../source hajonta/programs ui2d )
( cd build/debug && ./program ../../source hajonta/programs imgui )

# unit tests
${CLANG} ${CPPFLAGS} ${WARNFLAGS} -o build/debug/unit source/hajonta/bootstrap/unit.cpp ${DEBUG_FLAGS} ${INCLUDES}
( cd build/debug && ./unit )

# game
${CLANG} ${CPPFLAGS} ${WARNFLAGS} -c source/hajonta/game2.cpp ${DEBUG_FLAGS} ${INCLUDES} -o build/debug/game2.o
${CLANG} ${WARNFLAGS} -dynamiclib -o build/debug/libgame2.dylib build/debug/game2.o -framework OpenGL ${DEBUG_FLAGS}

# renderer
${CLANG} ${CPPFLAGS} ${WARNFLAGS} -c source/hajonta/renderer/opengl.cpp ${DEBUG_FLAGS} ${INCLUDES} -o build/debug/opengl.o
${CLANG} ${WARNFLAGS} -dynamiclib -o build/debug/libopenglrenderer.dylib build/debug/opengl.o -framework OpenGL ${DEBUG_FLAGS}

# binary
${CLANG} ${CPPFLAGS} ${WARNFLAGS} -F ~/Library/Frameworks/ -framework SDL2 -framework OpenGL -o build/debug/hajonta source/hajonta/platform/sdl2.cpp ${DEBUG_FLAGS} ${INCLUDES} -DHAJONTA_LIBRARY_NAME=libgame2.dylib -DHAJONTA_RENDERER_LIBRARY_NAME=libopenglrenderer.dylib
