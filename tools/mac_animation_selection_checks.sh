#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    mac_fp=
    if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
    objects=
    for unit in source/units/units.c source/objects/objects.c source/memory/data.c source/models/model_animations.c port/mac/engine_services.c port/mac/player_services.c port/mac/engine_animation_selection.c; do
        name=$(basename "$unit" .c)
        mac_prefix=port/mac/engine_prefix.h
        if [ "$unit" = source/units/units.c ] || [ "$unit" = source/objects/objects.c ]; then mac_prefix=port/mac/engine_xdk_prefix.h; fi
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $mac_fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files \
            -Isource/units -Isource/render -Isource/memory -Isource/game -Isource/input -Isource/objects -Iport/mac -Iport/linux/game -Iport/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
            -include "$mac_prefix" -include port/include/halo_math.h -include "$mac_dir/mac_engine_semantics.h" \
            -c "$unit" -o "$mac_dir/${name}_selection_${arch}.o"
        objects="$objects $mac_dir/${name}_selection_${arch}.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -fno-strict-aliasing -Iport/mac -Wl,-dead_strip \
        $objects port/mac/retail_cache.c port/mac/retail_animation.c port/mac/retail_animation_graph.c port/mac/retail_skeleton.c \
        tools/mac_engine_animation_selection_probe.c -o "$mac_dir/engine_animation_selection_$arch"
    nm -u "$mac_dir/engine_animation_selection_$arch" > "$mac_dir/animation_selection_imports_$arch.txt"
    if nm -m -u "$mac_dir/engine_animation_selection_$arch" | grep 'weak'; then
        echo "Unresolved weak engine import" >&2; exit 1
    fi
done
"$mac_dir/engine_animation_selection_i386" "$@"
