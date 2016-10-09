#!/bin/sh

set -e
set -u

CC=clang
WARNFLAGS="-Wall -Wno-c++11-compat-deprecated-writable-strings -Wno-unused-variable -Wno-missing-braces -Werror -ferror-limit=3 -Wno-unused-function -Wno-c++11-narrowing"
DEBUG_FLAGS="-DDEBUG -g"
CPPFLAGS="-std=c++14 -DHAJONTA_DEBUG=1 -DSDL_WITH_SUBDIR=1"
INCLUDES="-Isource -Ibuild/debug/generated -Ithirdparty/stb -Ithirdparty/imgui -Ithirdparty/par"
CLANG="clang++"

BASEDIR=`dirname $0`/../..
cd ${BASEDIR}
mkdir -p build
mkdir -p build/debug

ASSIMP_LIB_DIR=../thirdparty/assimp/lib
ASSIMP_LIB=${ASSIMP_LIB_DIR}/libassimp.dylib
ASSIMP_INCLUDE=../thirdparty/assimp/include

if [ -e ${ASSIMP_LIB} ]; then
    ${CLANG} ${CPPFLAGS} ${WARNFLAGS} -o build/debug/assimp_to_mesh_v1 source/hajonta/utils/assimp_to_mesh_v1.cpp ${DEBUG_FLAGS} ${INCLUDES} -I${ASSIMP_INCLUDE} -L${ASSIMP_LIB_DIR} -lassimp
else
    echo "[warn] No assimp lib found at ${ASSIMP_LIB}"
fi
