#!/bin/sh
# Link-closure audit only: forces real object_new into the stock constructor
# executable, but never runs it before its complete initialization exists.
# Prerequisites: mac_world_graph_checks.sh (musl), mac_config_checks.sh and
# mac_wide_checks.sh, mac_format_checks.sh, mac_socket_checks.sh and
# mac_handle_checks.sh (native settings and UTF-16 objects). Run those after
# changing their inputs before this gate; this gate never executes spawning.
# Uses standard port limits consistently across all units, including network
# declarations; this is broader than the isolated stock weapon-only gate.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
units='port/mac/native_geometry_math.c tools/mac_constructor_storage.c tools/mac_constructor_time.c source/items/items.c source/items/weapons.c source/memory/data.c source/memory/memory_pool.c port/mac/native_object_tags.c port/mac/engine_tags.c port/mac/engine_services.c port/mac/player_services.c port/mac/pool_services.c tools/mac_weapon_constructor_probe.c source/bitmaps/bitmap_utilities.c source/tag_files/tag_files.c source/main/console.c source/objects/object_types.c source/units/units.c source/units/bipeds.c source/models/models.c source/models/model_animations.c source/models/model_animation_definitions.c source/math/real_math.c source/math/geometry.c source/math/random_math.c source/math/periodic_functions.c source/scenario/scenario.c source/physics/bsp3d.c source/structures/structures.c source/structures/cluster_partitions.c source/structures/structure_bsp_definitions.c source/game/players.c port/mac/engine_game_engine.c port/mac/engine_common.c source/sound/game_sound.c source/objects/damage.c source/objects/object_lights.c source/effects/effects.c source/effects/contrails.c source/effects/particle_systems.c source/objects/widgets/widgets.c port/linux/game/object_bounds_cache.c port/linux/game/render_interpolation.c port/linux/game/network_objects.c'
cat > "$mac_dir/mac_spawn_witness_names.h" <<'NAMES'
#define MAC_SPAWN_JOIN_INNER(a,b) a##b
#define MAC_SPAWN_JOIN(a,b) MAC_SPAWN_JOIN_INNER(a,b)
#define verify_render_model_effect_size MAC_SPAWN_JOIN(mac_effect_witness_,__LINE__)
#define verify_rasterizer_model_begin_parameters_size MAC_SPAWN_JOIN(mac_begin_witness_,__LINE__)
#define effect_definition_size_assert MAC_SPAWN_JOIN(mac_effect_definition_witness_,__LINE__)
#define effect_event_definition_size_assert MAC_SPAWN_JOIN(mac_effect_event_witness_,__LINE__)
NAMES
units="$units ${MAC_EXTRA_UNITS:-}"
status=0
for arch in ${MAC_ARCHS:-ppc i386}; do
    fp=;if [ "$arch" = ppc ]; then fp=-mno-fused-madd;fi
    objects=;failed=0
    : > "$mac_dir/spawn_compile_failures_$arch.txt"
    for unit in $units; do
        name=$(basename "$unit" .c)
        # Explicit repair mode only: caller must verify all omitted units and
        # their headers/flags are unchanged since this same gate compiled them.
        if [ "${MAC_REUSE_UNCHANGED:-0}" = 1 ] && [ -f "$mac_dir/${name}_spawn_link_$arch.o" ]; then
            case " ${MAC_RECOMPILE_UNITS:-} " in
                *" $unit "*) ;;
                *) objects="$objects $mac_dir/${name}_spawn_link_$arch.o"; continue ;;
            esac
        fi
        alias=
        # The forced XDK prefix reads cseries.h before the implementation does.
        # Keep its CRT wrappers from recursively expanding to themselves.
        if [ "$unit" = source/cseries/cseries.c ]; then alias=-DBUILDING_CSERIES; fi
        if [ "$unit" = port/mac/engine_stack.c ]; then alias=-fexceptions; fi
        prefix="-include port/mac/engine_xdk_prefix.h"
        if [ "$unit" = source/rasterizer/rasterizer_geometry.c ]; then prefix="-include port/mac/engine_prefix.h -include port/mac/native_geometry_math.h -include port/mac/engine_xdk_prefix.h"; fi
        if [ "$unit" = port/mac/player_services.c ]; then alias=-Dglobal_structure_bsp_index=mac_spawn_unused_player_bsp_index;fi
        if [ "$unit" = source/ai/actor_type_grunt.c ]; then prefix="$prefix -include port/mac/engine_declspec_align.h"; fi
        if [ "$unit" = source/cseries/errors.c ]; then prefix="$prefix -include port/mac/engine_atomic_file.h"; fi
        if [ "$unit" = source/text/unicode.c ]; then prefix="$prefix -include port/mac/engine_wide_redirect.h"; fi
        # When the real cseries unit is linked, diagnostic CRT aliases must
        # not compete with its genuine implementations or temporary storage.
        case " $units " in *" source/cseries/cseries.c "*)
            case "$unit" in
                port/mac/engine_services.c) alias="$alias -Dcsmemcpy=mac_spawn_unused_memcpy -Dcsmemset=mac_spawn_unused_memset -Ddisplay_assert=mac_spawn_unused_assert" ;;
                port/mac/player_services.c) alias="$alias -Dcsmemcmp=mac_spawn_unused_memcmp -Dcsstrncpy=mac_spawn_unused_strncpy -Dcsprintf=mac_spawn_unused_printf -Dtemporary=mac_spawn_unused_temporary" ;;
                port/mac/pool_services.c) alias="$alias -Dcsmemmove=mac_spawn_unused_memmove" ;;
            esac ;;
        esac
        if [ "$unit" = port/mac/engine_services.c ]; then
            case " $units " in *" source/cseries/errors.c "*) alias="$alias -Derror=mac_spawn_unused_error" ;; esac
            case " $units " in *" source/tag_files/tag_groups.c "*) alias="$alias -Dtag_block_get_element_with_size=mac_spawn_unused_tag_block" ;; esac
        fi
        if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
            -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv $alias \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files -Isource/memory \
            -Isource/cache -Isource/camera -Isource/bitmaps -Isource/main -Isource/editor -Isource/networking -Isource/ai -Isource/interface -Isource/rasterizer -Isource/text -Isource/cutscene -Isource/shaders -Isource/game -Isource/input -Isource/objects -Isource/items -Isource/units -Isource/models \
            -Isource/scenario -Isource/structures -Isource/render -Isource/sound -Isource/effects -Isource/shell \
            -I"source/saved games" -Iport/mac -Iport/linux/game -idirafter port/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h $prefix \
            -include "$mac_dir/mac_spawn_witness_names.h" -include port/include/halo_math.h \
            -include port/linux/include/halo_linux_source_fixups.h \
            -include "$mac_dir/mac_engine_semantics.h" -c "$unit" -o "$mac_dir/${name}_spawn_link_$arch.o" \
            > "$mac_dir/${name}_spawn_link_$arch.txt" 2>&1; then
            objects="$objects $mac_dir/${name}_spawn_link_$arch.o"
        else
            echo "$unit" >> "$mac_dir/spawn_compile_failures_$arch.txt"
            failed=$((failed+1))
        fi
    done
    # Unchanged musl objects are generated by mac_world_graph_checks.sh.
    if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -Iport/mac \
        -Wl,-dead_strip -Wl,-u,_object_new $objects "$mac_dir"/*_musl_world_graph_${arch}.o \
        "$mac_dir/mac_shared_config_config_${arch}.o" "$mac_dir/engine_config_config_${arch}.o" "$mac_dir/tomlc17_config_${arch}.o" \
        "$mac_dir/mac_shared_wide_wide_${arch}.o" "$mac_dir/engine_wide_wide_${arch}.o" "$mac_dir/engine_format_${arch}.o" "$mac_dir/engine_sockets_${arch}.o" "$mac_dir/engine_handles_${arch}.o" \
        port/mac/retail_cache.c port/mac/retail_skeleton.c port/mac/retail_object_spawn.c port/mac/native_tags.c \
        -o "$mac_dir/spawn_link_$arch" > "$mac_dir/spawn_link_$arch.txt" 2>&1; then
        echo "$arch: link complete; $failed source compile failures; not executed"
    else
        echo "$arch: real object_new link incomplete; $failed source compile failures; see spawn_link_$arch.txt"
        status=1
    fi
    if [ "$failed" -ne 0 ]; then status=1;cat "$mac_dir/spawn_compile_failures_$arch.txt";fi
done
exit "$status"
