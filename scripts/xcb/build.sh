#!/bin/sh

set -e
set -u

CC="clang"
XCBLIBS="-lrt -lm -ldl -lxcb -lxcb-image -lxcb-icccm -lxcb-keysyms -lGL"
CPPFLAGS="-std=c++0x"
WARNFLAGS="-Wall -Wno-c++11-compat-deprecated-writable-strings -Wno-unused-variable -Werror -ferror-limit=3 -Wno-missing-braces"
DEBUG_FLAGS="-DDEBUG -g -DHAJONTA_DEBUG=1"
OPT_FLAGS="-Ofast"
INCLUDES="-Isource -Ibuild/debug/generated"

BASEDIR=`dirname $0`/../..
cd ${BASEDIR}
mkdir -p build
mkdir -p build/debug

${CC} ${CPPFLAGS} ${INCLUDES} ${WARNFLAGS} -o build/debug/program source/hajonta/bootstrap/program.cpp ${CPPFLAGS} ${DEBUG_FLAGS}
(cd build/debug && ./program ../../source hajonta/programs a )
(cd build/debug && ./program ../../source hajonta/programs debug_font )
(cd build/debug && ./program ../../source hajonta/programs b )

${CC} ${CPPFLAGS} ${INCLUDES} ${WARNFLAGS} -o build/debug/unit source/hajonta/bootstrap/unit.cpp ${CPPFLAGS} ${DEBUG_FLAGS} -lm
(cd build/debug && ./unit )

${CC} ${CPPFLAGS} -Isource -Ibuild/debug/generated -std=c++11 ${WARNFLAGS} -shared -Wl,-soname,libgame.so.1 -fPIC -o build/debug/libgame.so.new source/hajonta/game.cpp ${CPPFLAGS} ${DEBUG_FLAGS}
mv -f build/debug/libgame.so.new build/debug/libgame.so

${CC} ${CPPFLAGS} -Isource -std=c++0x ${WARNFLAGS} -o build/debug/xcb source/hajonta/platform/xcb.cpp ${CPPFLAGS} ${XCBLIBS} ${DEBUG_FLAGS}
