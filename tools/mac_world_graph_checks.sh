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
    for unit in source/scenario/scenario.c source/physics/bsp3d.c source/structures/structures.c source/structures/cluster_partitions.c source/memory/data.c port/mac/engine_world_partition.c source/math/real_math.c source/math/geometry.c source/structures/structure_bsp_definitions.c tools/mac_world_graph_probe.c port/mac/engine_services.c port/mac/player_services.c port/mac/native_scenario_objects.c port/mac/native_scenario_blocks.c port/mac/native_scenario_gameplay.c port/mac/native_scenario_scripts.c port/mac/native_scenario_skies.c port/mac/engine_script_cache.c port/mac/engine_world.c; do
        name=$(basename "$unit" .c)
        mac_prefix=port/mac/engine_prefix.h
        mac_alias=
        mac_defines=
        if [ "$unit" = source/scenario/scenario.c ] || [ "$unit" = source/structures/structures.c ]; then mac_prefix=port/mac/engine_xdk_prefix.h; fi
        # The graph probe and native service shims use Darwin stdio rather
        # than the shared Linux CRT wrappers.
        if [ "$unit" = tools/mac_world_graph_probe.c ] || [ "$unit" = port/mac/engine_services.c ] || [ "$unit" = port/mac/player_services.c ]; then mac_defines=-DHALO_LINUX_PLATFORM_LAYER; fi
        # scenario.c now owns this real global. The old player-only CRT slice
        # has a diagnostic placeholder; rename only that unused placeholder.
        if [ "$unit" = port/mac/player_services.c ]; then mac_alias=-Dglobal_structure_bsp_index=mac_world_graph_unused_player_bsp_index; fi
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $mac_fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv $mac_alias $mac_defines \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files -Isource/scenario -Isource/structures \
            -I"source/saved games" -Isource/units -Isource/render -Isource/memory -Isource/game -Isource/input -Isource/objects -Iport/mac -Iport/linux/game -Iport/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
            -include "$mac_prefix" -include port/include/halo_math.h -include "$mac_dir/mac_engine_semantics.h" \
            -c "$unit" -o "$mac_dir/${name}_world_graph_${arch}.o"
        objects="$objects $mac_dir/${name}_world_graph_${arch}.o"
    done
    for unit in port/third_party/musl-math/src/*.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu99 -O2 $mac_fp -Iport/third_party/musl-math/include \
            -include port/third_party/musl-math/include/libm.h \
            -c "$unit" -o "$mac_dir/${name}_musl_world_graph_${arch}.o"
        objects="$objects $mac_dir/${name}_musl_world_graph_${arch}.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -fno-strict-aliasing -Iport/mac -Wl,-dead_strip \
        $objects port/mac/retail_cache.c port/mac/retail_collision.c port/mac/retail_spawns.c \
        -o "$mac_dir/world_graph_$arch"
    nm -u "$mac_dir/world_graph_$arch" > "$mac_dir/world_graph_imports_$arch.txt"
    if nm -m -u "$mac_dir/world_graph_$arch" | grep 'weak'; then
        echo "Unresolved weak engine import" >&2; exit 1
    fi
done
"$mac_dir/world_graph_i386" "$@"
