set -xe

CC="clang"
CFLAGS="-std=c2x -Wall -Wextra -Wpedantic -g -O0 -DDEBUG_MODE -I./src -fPIC"
RAYLIB_FLAGS="$(pkg-config --libs --cflags raylib)"
BUILD_DIR="./build"
PROGRAM="logic-sim"
ENTRY="./src/main.c"
GAME_ENTRY="./src/game.c"

mkdir -p $BUILD_DIR

$CC $CFLAGS -shared $GAME_ENTRY -o "$BUILD_DIR/game.so.tmp" $RAYLIB_FLAGS
mv "$BUILD_DIR/game.so.tmp" "$BUILD_DIR/game.so"

$CC $CFLAGS -o "$BUILD_DIR/$PROGRAM" $ENTRY $RAYLIB_FLAGS -ldl
