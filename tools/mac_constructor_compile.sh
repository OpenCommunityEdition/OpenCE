#!/bin/sh
# Compile full engine constructors, then run native ABI witnesses. This does
# not link or run object constructors or claim a complete platform runtime.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    mac_fp=
    if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
    for unit in source/units/bipeds.c source/items/weapons.c tools/mac_spawn_layouts.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $mac_fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files \
            -Isource/units -Isource/items -Isource/render -Isource/memory -Isource/game -Isource/input -Isource/objects \
            -Iport/mac -Iport/linux/game -Iport/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
            -include port/mac/engine_xdk_prefix.h -include port/include/halo_math.h -include "$mac_dir/mac_engine_semantics.h" \
            -c "$unit" -o "$mac_dir/${name}_constructor_${arch}.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -Wl,-dead_strip "$mac_dir/mac_spawn_layouts_constructor_${arch}.o" -o "$mac_dir/spawn_layouts_$arch"
done
"$mac_dir/spawn_layouts_i386"
