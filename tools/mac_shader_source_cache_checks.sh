#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit/shader-source-cache}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    objects=
    for unit in port/mac/mac_shader_source_cache.c port/mac/mac_bone_gather.c port/mac/mac_glsl120.c \
        port/mac/mac_nv2a_vertex.c port/mac/mac_nv2a_pixel.c \
        port/mac/mac_xgpu_text.c \
        source/rasterizer/xbox/rasterizer_xbox_vertex_shaders.c \
        tools/mac_shader_source_cache_probe.c; do
        name=$(basename "$unit" .c)
        if [ "$unit" = port/mac/mac_bone_gather.c ]; then
            "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
                -std=gnu89 -O2 $fp -Iport/mac -c "$unit" -o "$mac_dir/${name}_$arch.o"
            objects="$objects $mac_dir/${name}_$arch.o"
            continue
        fi
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $fp -fms-extensions -fshort-wchar -fno-strict-aliasing \
            -fwrapv -I. -Isource -Isource/cseries -Isource/math -Iport/mac \
            -Iport/linux/src -Iport/include/xdk -idirafter port/linux/include \
            -include port/linux/include/halo_port_limits.h \
            -include port/linux/include/halo_port_capacity.h \
            -include port/mac/engine_xdk_prefix.h \
            -include build/mac-audit/mac_engine_semantics.h \
            -c "$unit" -o "$mac_dir/${name}_$arch.o"
        objects="$objects $mac_dir/${name}_$arch.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -Wl,-dead_strip $objects -framework OpenGL -o "$mac_dir/probe_$arch"
done
"$mac_dir/probe_i386"
