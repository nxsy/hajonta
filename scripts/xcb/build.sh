#!/bin/sh

set -e
set -u

XCBLIBS="-lrt -lm -ldl -lxcb -lxcb-image -lxcb-icccm -lxcb-keysyms -lGL"
CPPFLAGS="" # "-DGL_GLEXT_LEGACY"
WARNFLAGS="-Wall -Wno-unused-variable -Wno-write-strings"
GAMEWARNFLAGS="${WARNFLAGS} -Wno-sign-compare"
DEBUG_FLAGS="-DDEBUG -g"
OPT_FLAGS="-Ofast"

BASEDIR=`dirname $0`/../..
cd ${BASEDIR}
mkdir -p build
mkdir -p build/debug

g++ -std=c++0x ${WARNFLAGS} -o build/debug/program source/hajonta/bootstrap/program.cpp ${CPPFLAGS} ${DEBUG_FLAGS}
(cd build/debug && ./program ../../source hajonta/programs a )
g++ -Isource -Ibuild/debug/generated -std=c++0x ${GAMEWARNFLAGS} -shared -Wl,-soname,libgame.so.1 -fPIC -o build/debug/libgame.so.new source/hajonta/game.cpp ${CPPFLAGS} ${DEBUG_FLAGS}
mv -f build/debug/libgame.so.new build/debug/libgame.so
g++ -Isource -std=c++0x ${WARNFLAGS} -o build/debug/xcb source/hajonta/platform/xcb.cpp ${CPPFLAGS} ${XCBLIBS} ${DEBUG_FLAGS}
