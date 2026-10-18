#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
python tools/mac_resource_release_body.py build/mac-audit/mac_resource_release_body.inc
for arch in ppc i386; do
    fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
    objects=
    for unit in source/rasterizer/rasterizer_geometry.c source/math/real_math.c port/mac/engine_services.c port/mac/player_services.c port/mac/native_resource_memory.c port/mac/native_model_tags.c port/mac/native_model_buffers.c port/mac/native_geometry_math.c port/mac/engine_tags.c tools/mac_native_model_probe.c; do
        name=$(basename "$unit" .c)
        prefix="-include port/mac/engine_xdk_prefix.h"
        if [ "$unit" = source/rasterizer/rasterizer_geometry.c ];then prefix="-include port/mac/engine_prefix.h -include port/mac/native_geometry_math.h -include port/mac/engine_xdk_prefix.h";fi
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
            -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
            -I. -Isource -Isource/cseries -Isource/math -Isource/rasterizer -Isource/tag_files -Iport/mac -idirafter port/linux/include -Iport/include/xdk \
            $prefix -include port/linux/include/halo_port_capacity.h \
            -include port/include/halo_math.h -include "$mac_dir/mac_engine_semantics.h" \
            -c "$unit" -o "$mac_dir/${name}_native_model_$arch.o"
        objects="$objects $mac_dir/${name}_native_model_$arch.o"
    done
    for unit in port/third_party/musl-math/src/*.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu99 -O2 $fp \
            -Iport/third_party/musl-math/include -include port/third_party/musl-math/include/libm.h \
            -c "$unit" -o "$mac_dir/${name}_musl_native_model_$arch.o"
        objects="$objects $mac_dir/${name}_musl_native_model_$arch.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
        -Iport/mac -Wl,-dead_strip $objects port/mac/retail_cache.c port/mac/retail_skeleton.c port/mac/retail_object_spawn.c port/mac/native_tags.c \
        -o "$mac_dir/native_model_$arch"
    if nm -m -u "$mac_dir/native_model_$arch" | grep 'weak';then echo 'Unresolved weak import' >&2;exit 1;fi
    if nm -u "$mac_dir/native_model_$arch" | awk '{print $NF}' | grep -E '^_(sin|cos|tan|asin|acos|atan|atan2|exp|log|log2|log10|pow|sqrt|floor|ceil|sqrtf|floorf|lrintf)$';then echo 'System math dependency' >&2;exit 1;fi
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp port/mac/native_geometry_math.c tools/mac_geometry_math_probe.c -o "$mac_dir/geometry_math_$arch"
done
"$mac_dir/geometry_math_i386"
"$mac_dir/native_model_i386" "$@"
