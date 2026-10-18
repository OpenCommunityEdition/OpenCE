#!/bin/sh
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_PACKED_READBACK_OUT:-build/mac-audit/packed-readback}
mkdir -p "$out"
for arch in i386 ppc; do
    fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -Wall -Wextra tools/mac_packed_readback_probe.c \
        -framework OpenGL -o "$out/mac_packed_readback_$arch"
done
if [ "${MAC_PACKED_READBACK_BUILD_ONLY:-0}" != 1 ]; then
    "$out/mac_packed_readback_i386"
fi
