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
    for unit in source/scenario/scenario.c source/physics/bsp3d.c port/mac/engine_services.c port/mac/player_services.c port/mac/native_scenario_objects.c port/mac/native_scenario_blocks.c port/mac/native_scenario_gameplay.c port/mac/native_scenario_scripts.c port/mac/native_scenario_skies.c port/mac/engine_script_cache.c port/mac/engine_world.c; do
        name=$(basename "$unit" .c)
        mac_prefix=port/mac/engine_prefix.h
        mac_alias=
        mac_defines=
        if [ "$unit" = source/scenario/scenario.c ]; then mac_prefix=port/mac/engine_xdk_prefix.h; fi
        if [ "$unit" = port/mac/engine_services.c ]; then mac_defines=-DHALO_LINUX_PLATFORM_LAYER; fi
        if [ "$unit" = port/mac/player_services.c ]; then mac_alias=-Dglobal_structure_bsp_index=mac_world_unused_player_bsp_index; fi
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $mac_fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv $mac_alias $mac_defines \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files -Isource/scenario -Isource/structures \
            -Isource/units -Isource/render -Isource/memory -Isource/game -Isource/input -Isource/objects -Iport/mac -Iport/linux/game -Iport/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
            -include "$mac_prefix" -include port/include/halo_math.h -include "$mac_dir/mac_engine_semantics.h" \
            -c "$unit" -o "$mac_dir/${name}_scenario_gameplay_${arch}.o"
        objects="$objects $mac_dir/${name}_scenario_gameplay_${arch}.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -fno-strict-aliasing -I. -Isource -Isource/scenario -Isource/tag_files -Iport/mac -Wl,-dead_strip \
        $objects port/mac/retail_cache.c port/mac/retail_collision.c port/mac/retail_spawns.c \
        tools/mac_scenario_gameplay_probe.c -o "$mac_dir/scenario_gameplay_$arch"
done
"$mac_dir/scenario_gameplay_i386" "$@"
