#!/bin/sh
# Generated header must first be made with mac_engine_semantics.py on host.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    mac_fp=
    if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
    for unit in source/physics/bsp3d.c port/mac/engine_collision.c port/mac/engine_services.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $mac_fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files -Iport/mac \
            -include port/mac/engine_prefix.h -include "$mac_dir/mac_engine_semantics.h" \
            -c "$unit" -o "$mac_dir/${name}_${arch}.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -Iport/mac -Wl,-dead_strip \
        "$mac_dir/bsp3d_${arch}.o" "$mac_dir/engine_collision_${arch}.o" "$mac_dir/engine_services_${arch}.o" \
        port/mac/retail_cache.c port/mac/retail_collision.c port/mac/retail_spawns.c \
        tools/mac_engine_collision_probe.c -o "$mac_dir/engine_collision_$arch"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -Wall -Wextra -Werror \
        port/mac/mac_input.c tools/mac_input_probe.c -framework Carbon -o "$mac_dir/input_$arch"
done
"$mac_dir/engine_collision_i386" "$@"
"$mac_dir/input_i386"
