#!/bin/sh

set -e
set -u

CC=clang
WARNFLAGS="-Wall -Wno-c++11-compat-deprecated-writable-strings -Wno-unused-variable"
DEBUG_FLAGS="-DDEBUG -g"
CPPFLAGS="-std=c++0x -DHAJONTA_DEBUG=1"
INCLUDES="-Isource -Ibuild/debug/generated"

BASEDIR=`dirname $0`/../..
cd ${BASEDIR}
mkdir -p build
mkdir -p build/debug

# code generation
clang ${CPPFLAGS} ${WARNFLAGS} -o build/debug/program source/hajonta/bootstrap/program.cpp ${DEBUG_FLAGS}
( cd build/debug && ./program ../../source hajonta/programs a )
( cd build/debug && ./program ../../source hajonta/programs debug_font )

# game
clang ${CPPFLAGS} ${WARNFLAGS} -c source/hajonta/game.cpp ${DEBUG_FLAGS} ${INCLUDES} -o build/debug/game.o
clang ${WARNFLAGS} -dynamiclib -o build/debug/libgame.dylib build/debug/game.o -framework OpenGL ${DEBUG_FLAGS}

# binary
clang ${CPPFLAGS} ${WARNFLAGS} -framework Cocoa -framework QuartzCore -framework OpenGL -o build/debug/hajonta source/hajonta/platform/osx.mm ${DEBUG_FLAGS} ${INCLUDES}
