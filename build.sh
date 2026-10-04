#!/bin/sh
# Cross-build with the 32-bit MinGW-w64 toolchain (Linux or MSYS2).
set -e
cd "$(dirname "$0")"
i686-w64-mingw32-gcc -O2 -fno-omit-frame-pointer -Wall -Wextra -shared -static -static-libgcc \
    -o mss32.dll src/ds2pad.c src/mss32.def -lwinmm
