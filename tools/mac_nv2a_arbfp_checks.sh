#!/bin/sh
# Host checks of the NV2A pixel shader -> ARBfp1.0 translator (port/mac/mac_nv2a_arbfp.c):
# the frame's recovered pixel shader keys (regenerated and compared with the dumped
# GLSL when MAC_ARBFP_DUMP names the dump directory), every texture mode, fog mode,
# alpha test and combiner flag, refusals, random combiner words, with a validator
# of the ARBfp1.0 subset (syntax, declared temporaries, components read before
# written). No GPU and no game data. Run from the repository root with the legacy
# toolchain (MAC_CC, MAC_SDK as the other Mac checks); builds i386 and ppc and runs
# the i386 build. MAC_ARBFP_OUT is the output directory (the programs are saved there).
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_ARBFP_OUT:-build/mac-audit/arbfp}
dump=${MAC_ARBFP_DUMP:-}
mkdir -p "$out" "$out/programs"
for arch in i386 ppc; do
    fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    objects=
    for unit in port/mac/mac_nv2a_vertex.c port/mac/mac_nv2a_pixel.c port/mac/mac_xgpu_text.c \
        port/mac/mac_glsl120.c port/mac/mac_nv2a_arb.c port/mac/mac_nv2a_arbfp.c \
        source/rasterizer/xbox/rasterizer_xbox_vertex_shaders.c tools/mac_nv2a_arbfp_probe.c; do
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
        -framework OpenGL -o "$out/mac_nv2a_arbfp_probe_$arch"
done
"$out/mac_nv2a_arbfp_probe_i386" tools/mac_nv2a_arbfp_keys.txt $dump "$out/programs" 2>&1 | tee "$out/probe-i386.txt"
