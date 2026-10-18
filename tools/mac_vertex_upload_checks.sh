#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_out=${MAC_VERTEX_UPLOAD_OUT:-build/mac-audit/vertex-upload}
mkdir -p "$mac_out"
for arch in ppc i386; do
    fp=
    if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -Iport/mac \
        port/mac/mac_vertex_upload.c port/mac/mac_gl_program_cache.c \
        port/mac/mac_render_timing.c tools/mac_vertex_upload_probe.c \
        -framework OpenGL -o "$mac_out/mac_vertex_upload_$arch"
done
"$mac_out/mac_vertex_upload_i386"
