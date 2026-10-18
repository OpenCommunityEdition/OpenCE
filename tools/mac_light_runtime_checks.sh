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
    for unit in port/mac/engine_arena.c tools/mac_light_runtime_probe.c source/objects/object_lights.c source/structures/cluster_partitions.c source/memory/data.c source/memory/memory_pool.c source/memory/crc.c port/mac/engine_services.c port/mac/player_services.c port/mac/pool_services.c; do
        name=$(basename "$unit" .c)
        mac_prefix=port/mac/engine_xdk_prefix.h
        mac_alias=
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $mac_fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv $mac_alias \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files -Isource/scenario -Isource/structures \
            -I"source/saved games" -Isource/units -Isource/render -Isource/sound -Isource/main -Isource/interface -Isource/rasterizer -Isource/cutscene -Isource/ai -Isource/cache -Isource/camera -Isource/bitmaps -Isource/models -Isource/items -Isource/effects -Isource/memory -Isource/game -Isource/input -Isource/objects -Iport/mac -Iport/linux/game -idirafter port/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
            -include "$mac_prefix" -include port/include/halo_math.h -include "$mac_dir/mac_engine_semantics.h" \
            -c "$unit" -o "$mac_dir/${name}_light_runtime_${arch}.o"
        objects="$objects $mac_dir/${name}_light_runtime_${arch}.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -fno-strict-aliasing -Iport/mac -Wl,-dead_strip \
        $objects \
        -o "$mac_dir/light_runtime_$arch"
    nm -u "$mac_dir/light_runtime_$arch" > "$mac_dir/light_runtime_imports_$arch.txt"
    if nm -m -u "$mac_dir/light_runtime_$arch" | grep 'weak'; then
        echo "Unresolved weak engine import" >&2; exit 1
    fi
done
"$mac_dir/light_runtime_i386" "$@"
