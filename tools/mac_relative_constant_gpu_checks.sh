#!/bin/sh
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_RELATIVE_CONSTANT_OUT:-build/mac-audit/relative-constant-gpu}
mkdir -p "$out"
for arch in i386 ppc; do
    fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    objects=
    for unit in port/mac/mac_nv2a_vertex.c port/mac/mac_xgpu_text.c \
        port/mac/mac_bone_gather.c source/rasterizer/xbox/rasterizer_xbox_vertex_shaders.c \
        port/mac/mac_glsl120.c tools/mac_relative_constant_gpu_probe.c; do
        name=$(basename "$unit" .c)
        prefix="-include port/mac/engine_xdk_prefix.h"
        if [ "$unit" = port/mac/mac_bone_gather.c ]; then prefix=; fi
        "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $fp -Wall -Wextra -fms-extensions -fshort-wchar \
            -fno-strict-aliasing -fwrapv -I. -Isource -Isource/cseries \
            -Isource/math -Iport/mac -Iport/linux/src -Iport/include/xdk \
            -idirafter port/linux/include \
            -include port/linux/include/halo_port_limits.h \
            -include port/linux/include/halo_port_capacity.h \
            $prefix \
            -include build/mac-audit/mac_engine_semantics.h \
            -c "$unit" -o "$out/${name}_$arch.o"
        objects="$objects $out/${name}_$arch.o"
    done
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -Wl,-dead_strip $objects -framework AGL -framework OpenGL \
        -o "$out/mac_relative_constant_gpu_$arch"
done
if [ "${MAC_RELATIVE_CONSTANT_BUILD_ONLY:-0}" != 1 ]; then
    "$out/mac_relative_constant_gpu_i386" >"$out/runtime-i386.txt" 2>&1
fi
