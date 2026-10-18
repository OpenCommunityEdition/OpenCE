#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit/gl-program-cache}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    mac_fp=; if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -Wall -Wextra -Iport/mac \
        tools/mac_gl_program_cache_probe.c port/mac/mac_gl_program_cache.c port/mac/mac_render_timing.c \
        -framework AGL -framework OpenGL -o "$mac_dir/mac_gl_program_cache_$arch"
done
"$mac_dir/mac_gl_program_cache_i386"
