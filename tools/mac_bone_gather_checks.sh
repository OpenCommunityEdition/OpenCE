#!/bin/sh
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_BONE_GATHER_OUT:-build/mac-audit/bone-gather}
mkdir -p "$out"
for arch in i386 ppc; do
    fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -Wall -Wextra -I. -Iport/mac \
        port/mac/mac_bone_gather.c tools/mac_bone_gather_probe.c \
        -o "$out/mac_bone_gather_$arch"
done
"$out/mac_bone_gather_i386" >"$out/runtime-i386.txt" 2>&1
cat "$out/runtime-i386.txt"
