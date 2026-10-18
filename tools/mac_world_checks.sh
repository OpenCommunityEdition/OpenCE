#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mac_probe=${MAC_WORLD_PROBE:-tools/mac_engine_world_probe.c}
mac_exe=${MAC_WORLD_EXE:-engine_world}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    mac_fp=
    if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
    objects=
    for unit in source/scenario/scenario.c source/physics/bsp3d.c source/rasterizer/rasterizer_geometry.c port/mac/retail_geometry.c port/mac/engine_services.c port/mac/player_services.c port/mac/native_scenario_objects.c port/mac/native_scenario_blocks.c port/mac/native_scenario_gameplay.c port/mac/native_scenario_scripts.c port/mac/native_scenario_skies.c port/mac/engine_script_cache.c port/mac/engine_world.c; do
        name=$(basename "$unit" .c)
        mac_prefix=port/mac/engine_prefix.h
        mac_alias=
        mac_defines=
        if [ "$unit" = source/scenario/scenario.c ]; then mac_prefix=port/mac/engine_xdk_prefix.h; fi
        # Native platform services use Darwin stdio directly; the port's
        # Linux CRT aliases are for the shared engine source only.
        if [ "$unit" = port/mac/engine_services.c ]; then mac_defines=-DHALO_LINUX_PLATFORM_LAYER; fi
        # scenario.c now owns this real global. The old player-only CRT slice
        # has a diagnostic placeholder; rename only that unused placeholder.
        if [ "$unit" = port/mac/player_services.c ]; then mac_alias=-Dglobal_structure_bsp_index=mac_world_unused_player_bsp_index; fi
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $mac_fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv $mac_alias $mac_defines \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files -Isource/scenario -Isource/structures \
            -Isource/units -Isource/render -Isource/memory -Isource/game -Isource/input -Isource/objects -Iport/mac -Iport/linux/game -Iport/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
            -include "$mac_prefix" -include port/include/halo_math.h -include "$mac_dir/mac_engine_semantics.h" \
            -c "$unit" -o "$mac_dir/${name}_world_${arch}.o"
        objects="$objects $mac_dir/${name}_world_${arch}.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
        -DHALO_LINUX_PLATFORM_LAYER -I. -Isource -Isource/cseries -Isource/math -Isource/physics \
        -Isource/tag_files -Isource/scenario -Isource/structures -Isource/units -Isource/render \
        -Isource/memory -Isource/game -Isource/input -Isource/objects -Iport/mac -Iport/linux/game \
        -Iport/linux/include -Iport/include/xdk -include port/linux/include/halo_port_limits.h \
        -include port/linux/include/halo_port_capacity.h -include port/mac/engine_xdk_prefix.h \
        -include port/include/halo_math.h -include "$mac_dir/mac_engine_semantics.h" \
        -c "$mac_probe" -o "$mac_dir/${mac_exe}_probe_${arch}.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -fno-strict-aliasing -I. -Isource -Isource/cseries -Isource/math \
        -Isource/physics -Isource/tag_files -Isource/scenario -Isource/structures -Isource/units \
        -Isource/render -Isource/memory -Isource/game -Isource/input -Isource/objects \
        -Iport/mac -Iport/linux/game -Iport/linux/include -Iport/include/xdk -Wl,-dead_strip \
        $objects "$mac_dir/${mac_exe}_probe_${arch}.o" port/mac/retail_cache.c port/mac/retail_collision.c port/mac/retail_spawns.c \
        -o "$mac_dir/${mac_exe}_$arch"
    nm -u "$mac_dir/${mac_exe}_$arch" > "$mac_dir/${mac_exe}_imports_$arch.txt"
    if nm -m -u "$mac_dir/${mac_exe}_$arch" | grep 'weak'; then
        echo "Unresolved weak engine import" >&2; exit 1
    fi
done
"$mac_dir/${mac_exe}_i386" "$@"
