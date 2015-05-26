#!/bin/sh

set -e
set -u

CC=clang
WARNFLAGS="-Wall -Wno-c++11-compat-deprecated-writable-strings -Wno-unused-variable -Wno-missing-braces -Werror -ferror-limit=3 -Wno-unused-function"
DEBUG_FLAGS="-DDEBUG -g"
CPPFLAGS="-std=c++0x -DHAJONTA_DEBUG=1"
INCLUDES="-Isource -Ibuild/debug/generated"

BASEDIR=`dirname $0`/../..
cd ${BASEDIR}
mkdir -p build
mkdir -p build/debug

# code generation
clang ${CPPFLAGS} ${WARNFLAGS} -o build/debug/program source/hajonta/bootstrap/program.cpp ${DEBUG_FLAGS} ${INCLUDES}
( cd build/debug && ./program ../../source hajonta/programs a )
( cd build/debug && ./program ../../source hajonta/programs debug_font )
( cd build/debug && ./program ../../source hajonta/programs b )
( cd build/debug && ./program ../../source hajonta/programs ui2d )
( cd build/debug && ./program ../../source hajonta/programs skybox )
( cd build/debug && ./program ../../source hajonta/programs c )

# unit tests
clang ${CPPFLAGS} ${WARNFLAGS} -o build/debug/unit source/hajonta/bootstrap/unit.cpp ${DEBUG_FLAGS} ${INCLUDES}
( cd build/debug && ./unit )

# util 
clang ${CPPFLAGS} ${WARNFLAGS} -o build/debug/bump_to_normal source/hajonta/utils/bump_to_normal.cpp ${DEBUG_FLAGS} ${INCLUDES} -Isource/hajonta/thirdparty

# game
clang ${CPPFLAGS} ${WARNFLAGS} -c source/hajonta/game.cpp ${DEBUG_FLAGS} ${INCLUDES} -o build/debug/game.o
clang ${WARNFLAGS} -dynamiclib -o build/debug/libgame.dylib build/debug/game.o -framework OpenGL ${DEBUG_FLAGS}

# editor
clang ${CPPFLAGS} ${WARNFLAGS} -c source/hajonta/utils/editor.cpp ${DEBUG_FLAGS} ${INCLUDES} -o build/debug/editor.o
clang ${WARNFLAGS} -dynamiclib -o build/debug/libeditor.dylib build/debug/editor.o -framework OpenGL ${DEBUG_FLAGS}

# ddsviewer
clang ${CPPFLAGS} ${WARNFLAGS} -c source/hajonta/utils/ddsviewer.cpp ${DEBUG_FLAGS} ${INCLUDES} -o build/debug/ddsviewer.o
clang ${WARNFLAGS} -dynamiclib -o build/debug/libddsviewer.dylib build/debug/ddsviewer.o -framework OpenGL ${DEBUG_FLAGS}

# binary
clang ${CPPFLAGS} ${WARNFLAGS} -framework Cocoa -framework QuartzCore -framework OpenGL -o build/debug/hajonta source/hajonta/platform/osx.mm ${DEBUG_FLAGS} ${INCLUDES}
clang ${CPPFLAGS} ${WARNFLAGS} -framework Cocoa -framework QuartzCore -framework OpenGL -o build/debug/editor source/hajonta/platform/osx.mm ${DEBUG_FLAGS} ${INCLUDES} -DHAJONTA_LIBRARY_NAME=libeditor.dylib
clang ${CPPFLAGS} ${WARNFLAGS} -framework Cocoa -framework QuartzCore -framework OpenGL -o build/debug/ddsviewer source/hajonta/platform/osx.mm ${DEBUG_FLAGS} ${INCLUDES} -DHAJONTA_LIBRARY_NAME=libddsviewer.dylib
