#!/bin/sh
# GPU comparison of the NV2A pixel shader -> ARBfp1.0 translator (port/mac/mac_nv2a_arbfp.c) with the
# GLSL fragment program path, for the pixel shaders of a real frame, synthetic states and random
# combiner words (tools/mac_nv2a_arbfp_gpu_probe.c; MAC_ARBFP_GPU_ARGS takes its arguments).
# Run from the repository root with the legacy toolchain; needs no game data.
# Builds i386 and ppc; runs the i386 build unless MAC_ARBFP_GPU_BUILD_ONLY=1.
# On a G4, copy the ppc binary and tools/mac_nv2a_arbfp_keys.txt and run it (hidden CGL context, no window).
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_ARBFP_GPU_OUT:-build/mac-audit/arbfp-gpu}
mkdir -p "$out"
for arch in i386 ppc; do
    fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    objects=
    for unit in port/mac/mac_nv2a_vertex.c port/mac/mac_nv2a_pixel.c port/mac/mac_xgpu_text.c \
        port/mac/mac_glsl120.c port/mac/mac_nv2a_arb.c port/mac/mac_nv2a_arbfp.c \
        source/rasterizer/xbox/rasterizer_xbox_vertex_shaders.c tools/mac_nv2a_arbfp_gpu_probe.c; do
        name=$(basename "$unit" .c)
        prefix="-include port/mac/engine_xdk_prefix.h"
        case "$unit" in port/mac/mac_nv2a_arb.c) prefix= ;; esac
        "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $fp -Wall -Wextra -Wno-unused-parameter -fms-extensions -fshort-wchar \
            -fno-strict-aliasing -fwrapv -I. -Isource -Isource/cseries -Isource/math -Iport/mac \
            -Iport/linux/src -Iport/include/xdk -idirafter port/linux/include \
            -include port/linux/include/halo_port_limits.h \
            -include port/linux/include/halo_port_capacity.h $prefix \
            -include build/mac-audit/mac_engine_semantics.h \
            -c "$unit" -o "$out/${name}_$arch.o"
        objects="$objects $out/${name}_$arch.o"
    done
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -Wl,-dead_strip $objects \
        -framework OpenGL -o "$out/mac_nv2a_arbfp_gpu_$arch"
done
if [ "${MAC_ARBFP_GPU_BUILD_ONLY:-0}" != 1 ]; then
    "$out/mac_nv2a_arbfp_gpu_i386" tools/mac_nv2a_arbfp_keys.txt ${MAC_ARBFP_GPU_ARGS:-} >"$out/runtime-i386.txt" 2>&1 || true
    tail -5 "$out/runtime-i386.txt"
fi
