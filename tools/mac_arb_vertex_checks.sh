#!/bin/sh
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_ARB_VERTEX_OUT:-build/mac-audit/arb-vertex}
mkdir -p "$out"
for arch in i386 ppc; do
    fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -Wall -Wextra \
        tools/mac_arb_vertex_probe.c -framework AGL -framework OpenGL \
        -o "$out/mac_arb_vertex_$arch"
done
"$out/mac_arb_vertex_i386" >"$out/runtime-i386.txt" 2>&1
