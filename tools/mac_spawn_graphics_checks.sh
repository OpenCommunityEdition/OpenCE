#!/bin/sh
# Read-only extension of an already compiled genuine spawning baseline.
# Only compile/link evidence; no executable is run and no desktop is opened.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
base=${MAC_BASELINE_DIR:-build/mac-audit}
out=${MAC_GRAPHICS_DIR:-build/mac-graphics-audit}
mkdir -p "$out"
# Generate on the modern host with tools/mac_frame_statistics_boundary.py and
# stage this header before compiling on the GCC 4.0 MacBook.
test -f "$out/mac_frame_statistics_body.h"
units='source/rasterizer/xbox/rasterizer_xbox_models.c source/rasterizer/xbox/rasterizer_xbox_decals.c source/rasterizer/xbox/rasterizer_xbox_draw_primitives.c source/rasterizer/xbox/rasterizer_xbox_dynavobgeom.c source/rasterizer/xbox/rasterizer_xbox_widgets.c source/rasterizer/xbox/rasterizer_xbox_profile.c source/rasterizer/xbox/rasterizer_xbox.c source/rasterizer/rasterizer_debug.c source/rasterizer/rasterizer_text.c source/rasterizer/xbox/rasterizer_xbox_text.c source/memory/lruv_cache.c source/rasterizer/xbox/rasterizer_xbox_environment_fog.c source/rasterizer/rasterizer_transparent_geometry.c source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c source/interface/progress_bar.c source/text/international_strings.c port/mac/engine_frame_statistics.c source/rasterizer/rasterizer_memory_pool.c source/rasterizer/rasterizer_swizzle.c port/linux/game/custom_edition_geometry.c source/text/font_group.c source/rasterizer/common/rasterizer_common.c source/rasterizer/xbox/rasterizer_xbox_active_camouflage.c source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.c source/rasterizer/xbox/rasterizer_xbox_vertex_shaders_runtime.c source/render/render.c source/cache/xbox_sound_cache.c port/mac/engine_texture_cache.c port/mac/native_bitmap_tags.c port/mac/retail_texture.c port/mac/engine_render_globals.c source/rasterizer/xbox/rasterizer_xbox_errors.c source/rasterizer/xbox/rasterizer_xbox_plasma_energy.c source/rasterizer/xbox/rasterizer_xbox_water.c source/rasterizer/xbox/shader_transparent_chicago_preprocessor.c source/rasterizer/xbox/shader_transparent_generic_preprocessor.c source/rasterizer/xbox/rasterizer_xbox_vertex_shaders.c port/linux/game/shield_color.c port/linux/game/mod_sounds.c port/mac/engine_yield.c port/mac/engine_player_profile.c'
baseline=$(sed -n "s/^units='\(.*\)'$/\1/p" tools/mac_spawn_link_checks.sh)
extra=$(sed -n "s/^MAC_EXTRA_UNITS='\(.*\)'$/\1/p" tools/mac_spawn_provider_checks.sh)
for arch in ${MAC_ARCHS:-ppc i386}; do
    fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
    objects=;failures=0
    : > "$out/compile_failures_$arch.txt"
    for unit in $units;do
        name=$(basename "$unit" .c)
        if [ "${MAC_REUSE_GRAPHICS:-0}" = 1 ] && [ -f "$out/${name}_$arch.o" ]; then
            case " ${MAC_GRAPHICS_RECOMPILE:-} " in
                *" $unit "*) ;;
                *) objects="$objects $out/${name}_$arch.o"; continue ;;
            esac
        fi
        extra_prefix=
        if [ "$unit" = source/rasterizer/rasterizer_text.c ];then extra_prefix="-include port/mac/engine_rasterizer_text_prefix.h";fi
        if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
            -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files -Isource/memory \
            -Isource/cache -Isource/camera -Isource/bitmaps -Isource/main -Isource/editor -Isource/networking -Isource/ai -Isource/interface -Isource/rasterizer -Isource/rasterizer/xbox -Isource/text -Isource/cutscene -Isource/shaders -Isource/game -Isource/input -Isource/objects -Isource/items -Isource/units -Isource/models \
            -Isource/scenario -Isource/structures -Isource/render -Isource/sound -Isource/effects -Isource/shell -Isource/bink \
            -I"source/saved games" -I"$out" -Iport/mac -Iport/linux/game -idirafter port/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h -include port/mac/engine_xdk_prefix.h \
            -include "$base/mac_spawn_witness_names.h" -include port/include/halo_math.h \
            -include port/linux/include/halo_linux_source_fixups.h -include "$base/mac_engine_semantics.h" $extra_prefix \
            -c "$unit" -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1;then
            objects="$objects $out/${name}_$arch.o"
        else echo "$unit" >> "$out/compile_failures_$arch.txt";failures=$((failures+1));fi
    done
    for unit in $baseline $extra;do name=$(basename "$unit" .c);objects="$objects $base/${name}_spawn_link_$arch.o";done
    for input in $objects "$base"/*_musl_world_graph_${arch}.o          "$base/mac_shared_config_config_${arch}.o" "$base/engine_config_config_${arch}.o" "$base/tomlc17_config_${arch}.o"          "$base/mac_shared_wide_wide_${arch}.o" "$base/engine_wide_wide_${arch}.o" "$base/engine_format_${arch}.o" "$base/engine_sockets_${arch}.o" "$base/engine_handles_${arch}.o" "$base/engine_device_state_device_state_${arch}.o"          "$base/shared_d3d_resources_cpu_resources_${arch}.o" "$base/shared_texture_layout_cpu_resources_${arch}.o" "$base/native_resource_memory_cpu_resources_${arch}.o" "$base/engine_text_text_${arch}.o" "$base/mac_text_fonts_text_${arch}.o" "$base/engine_cache_files_${arch}.o" "$base/mapped_file_cache_files_${arch}.o"; do echo "$input"; done > "$out/objects_$arch.txt"
    printf "%s\n" port/mac/retail_cache.c port/mac/retail_skeleton.c port/mac/retail_object_spawn.c port/mac/native_tags.c > "$out/source_inputs.txt"
    if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -Iport/mac \
        -Wl,-dead_strip -Wl,-u,_object_new $objects "$base"/*_musl_world_graph_${arch}.o \
        "$base/mac_shared_config_config_${arch}.o" "$base/engine_config_config_${arch}.o" "$base/tomlc17_config_${arch}.o" \
        "$base/mac_shared_wide_wide_${arch}.o" "$base/engine_wide_wide_${arch}.o" "$base/engine_format_${arch}.o" "$base/engine_sockets_${arch}.o" "$base/engine_handles_${arch}.o" "$base/engine_device_state_device_state_${arch}.o" \
        "$base/shared_d3d_resources_cpu_resources_${arch}.o" "$base/shared_texture_layout_cpu_resources_${arch}.o" "$base/native_resource_memory_cpu_resources_${arch}.o" "$base/engine_text_text_${arch}.o" "$base/mac_text_fonts_text_${arch}.o" "$base/engine_cache_files_${arch}.o" "$base/mapped_file_cache_files_${arch}.o" \
        port/mac/retail_cache.c port/mac/retail_skeleton.c port/mac/retail_object_spawn.c port/mac/native_tags.c \
        -o "$out/spawn_graphics_$arch" > "$out/link_$arch.txt" 2>&1;then
        echo "$arch: linked; not executed; $failures compile failures"
    else echo "$arch: link incomplete; $failures compile failures";fi
    cat "$out/compile_failures_$arch.txt"
done
