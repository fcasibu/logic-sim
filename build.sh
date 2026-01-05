set -xe

CC="clang"
CFLAGS="-std=c2x -Wall -Wextra -Wpedantic -g -O0 -DDEBUG_MODE -I./src"
RAYLIB_FLAGS="$(pkg-config --libs --cflags raylib)"
BUILD_DIR="./build"
PROGRAM="logic-sim"
ENTRY="./src/main.c"

mkdir -p $BUILD_DIR

$CC $CFLAGS -o "$BUILD_DIR/$PROGRAM" $ENTRY $RAYLIB_FLAGS
