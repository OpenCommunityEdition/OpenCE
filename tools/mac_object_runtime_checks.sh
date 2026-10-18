#!/bin/sh
# Reuses the verified graphics manifest, excluding its diagnostic main.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
base=build/mac-audit
manifest=${MAC_GRAPHICS_MANIFEST:-build/mac-scenery-view-fixed}
out=${MAC_RUNTIME_OUT:-build/mac-object-runtime}
probe=${MAC_RUNTIME_PROBE:-tools/mac_object_runtime_probe.c}
session_units=
if [ "${MAC_RUNTIME_SESSION:-0}" = 1 ];then
    session_units="port/mac/engine_cache_files.c port/mac/engine_session.c port/mac/native_environment_assets.c port/mac/native_effect_assets.c"
fi
mkdir -p "$out"
python tools/mac_object_renderer_boundary.py
python tools/mac_model_base_bitmap_boundary.py
python tools/mac_xgpu_texture_decode_view.py "$base/mac_xgpu_texture_decode_view.inc"
apply_flags=;apply_units=;vehicle_units=;sky_units=;native_sky_units=;native_input_units=;model_gpu_units=;gpu_support_units=;menu_units=port/mac/engine_menu_state.c
if [ "${3:-}" = --player-apply ] || [ "${3:-}" = --player-movement ] || [ "${3:-}" = --object-update ] || [ "${3:-}" = --decal-lifecycle ] || [ "${3:-}" = --vehicle-object-update ];then
    apply_flags=-DMAC_PLAYER_QUEUE_APPLY
    apply_units="tools/mac_player_movement_probe.c"
fi
if [ "${3:-}" = --vehicle-object-update ];then
    if [ "$#" -ne 4 ] || [ ! -f "$4" ];then
        echo "vehicle object update requires: <map> <bitmaps.map> --vehicle-object-update <sounds.map>" >&2
        exit 2
    fi
    apply_flags="$apply_flags -DMAC_VEHICLE_RUNTIME"
    vehicle_units="tools/mac_vehicle_runtime_probe.c"
fi
if [ "${3:-}" = --sky-assets ];then
    apply_flags="$apply_flags -DMAC_SKY_ASSETS"
    sky_units="tools/mac_sky_assets_runtime_probe.c"
    native_sky_units="port/mac/native_sky_tags.c port/mac/native_sky_assets.c port/mac/engine_cache_files.c"
fi
if [ "${3:-}" = --model-gpu ];then
    apply_flags="$apply_flags -DMAC_MODEL_GPU"
    model_gpu_units="port/mac/native_model_gpu_probe.c"
    gpu_support_units="port/mac/engine_texture_cache.c port/mac/engine_screen.c port/mac/native_resource_memory.c port/mac/mac_glsl120.c port/mac/mac_xgpu_text.c port/mac/mac_nv2a_vertex.c port/mac/mac_nv2a_pixel.c port/mac/mac_gpu_textures.c port/mac/mac_shader_source_cache.c port/mac/mac_bone_gather.c port/mac/mac_gl_visibility.c port/mac/mac_gl_program_cache.c port/mac/mac_render_timing.c port/mac/mac_vertex_upload.c port/mac/mac_index_decode_cache.c port/mac/bsp_decode_cache.c port/mac/engine_cache_files.c source/interface/player_ui.c"
fi
if [ "${HALO_MAC_NATIVE_INPUT:-0}" = 1 ];then
    apply_flags="$apply_flags -DMAC_NATIVE_INPUT"
    native_input_units="source/input/input_abstraction.c port/mac/engine_input.c port/mac/engine_console.c source/interface/player_ui.c source/interface/ui_widget.c source/interface/virtual_keyboard.c source/interface/event_manager.c source/memory/stack_memory_pool.c source/interface/ui_widget_event_handler_functions.c source/cache/cache_files.c source/hs/hs.c port/linux/game/menu_tags.c"
    menu_units=
fi
view_frameworks=
input_frameworks=
if [ "${HALO_MAC_NATIVE_INPUT:-0}" = 1 ];then input_frameworks="-framework Carbon -framework CoreAudio -framework CoreFoundation -framework ApplicationServices";fi
if [ "${MAC_RUNTIME_VIEW:-0}" = 1 ];then
    apply_flags="$apply_flags -DMAC_RUNTIME_VIEW"
    view_frameworks="-framework Carbon -framework AGL -framework OpenGL"
    if [ -z "$gpu_support_units" ];then
        gpu_support_units="port/mac/engine_texture_cache.c port/mac/engine_screen.c port/mac/native_resource_memory.c port/mac/mac_glsl120.c port/mac/mac_xgpu_text.c port/mac/mac_nv2a_vertex.c port/mac/mac_nv2a_pixel.c port/mac/mac_gpu_textures.c port/mac/mac_shader_source_cache.c port/mac/mac_bone_gather.c port/mac/mac_gl_visibility.c port/mac/mac_gl_program_cache.c port/mac/mac_render_timing.c port/mac/mac_vertex_upload.c port/mac/engine_cache_files.c source/interface/player_ui.c"
    fi
fi
if [ "${3:-}" = --model-gpu ];then
    view_frameworks="-framework Carbon -framework AGL -framework OpenGL"
fi
# Generate this narrow headless owner locally before staging on legacy Python.
# tools/mac_menu_state_boundary.py writes mac_menu_state_body.h.
# python3 tools/mac_game_state_boundary.py build/mac-audit/mac_game_state_body.h
# python3 tools/mac_bitmap_sampling_boundary.py build/mac-audit/mac_engine_bitmaps_body.h
# Run the generators on the modern host and stage the headers with this script.
for arch in ${MAC_ARCHS:-ppc i386}; do
    fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
    objects=
    for file in $(cat "$manifest/objects_$arch.txt"); do
        if [ "${MAC_RUNTIME_SESSION:-0}" = 1 ];then
            case "$file" in */cache_files_*) continue ;; esac
        fi
        case "$file" in *hud_messaging_*|*/hud_unit_*|*/structure_runtime_decals_*|*/hud_*|*/mac_hud_runtime_probe_*|*/mac_scenery_runtime_probe_*|*/mac_vehicle_runtime_probe_*|*/mac_object_runtime_probe_*|*/mac_player_queue_probe_*|*/mac_player_movement_probe_*|*/mac_window_*|*/mac_input_*|*/mac_bsp_renderer_*|*/mac_runtime_view_*|*/native_model_gpu_probe_*|*/engine_texture_cache_*|*/native_resource_memory_*|*/player_queues_new_*|*/object_deleted_procs_*|*/object_mesh_*|*/render_objects_*|*/game_spawn_link_*|*/engine_arena_*|*/engine_services_*|*/engine_device_state_*|*/engine_game_runtime_*|*/engine_bitmaps_*|*/engine_menu_state_*|*/engine_handles_*|*/engine_cache_files_*|*/engine_sockets_*|*/engine_clock_*|*/transport_endpoint_winsock_*) continue ;; esac
        case "$file" in
            *mac_weapon_constructor_probe*|*/mapped_file_*|*/retail_texture_*|*/retail_cache_*|*/retail_skeleton_*|*/retail_object_spawn_*|*/native_tags_*|*/retail_collision_model_*|*/retail_collision_*|*/retail_animation_*|*/retail_animation_graph_*|*/engine_world_*|*/native_actor_assets_*|*/native_decal_assets_*|*/native_sky_assets_*|*/native_sky_tags_*|*/native_physics_tags_*|*/native_antenna_tags_*|*/native_bitmap_tags_*|*/native_object_tags_*|*/native_light_tags_*|*/native_collision_tags_*|*/native_animation_tags_*|*/native_model_tags_*|*/native_model_buffers_*|*/first_person_legs_*) ;;
            *) objects="$objects $file" ;;
        esac
    done
    # Native OS translation units own their system headers and must not receive
    # the game's forced prefix (Winsock/timeval and libc names differ).
    os_units="port/mac/engine_handles.c port/mac/engine_sockets.c"
    if [ "${3:-}" = --model-gpu ];then os_units="$os_units port/mac/mac_window.c port/mac/mac_input.c";fi
    if [ "${3:-}" = --model-gpu ];then os_units="$os_units port/mac/engine_messages.c";fi
    if [ "${MAC_RUNTIME_VIEW:-0}" = 1 ] && [ "${3:-}" != --model-gpu ];then os_units="$os_units port/mac/engine_messages.c";fi
    if [ "${HALO_MAC_NATIVE_INPUT:-0}" = 1 ];then os_units="$os_units port/mac/mac_input.c port/mac/engine_clipboard.c port/mac/engine_audio_devices.c port/mac/engine_display.c port/mac/engine_messages.c";fi
    for unit in $os_units; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $fp -D__int64='long long' -D__fastcall= \
            -Iport/mac -Iport/include/xdk -Iport/linux/include \
            -c "$unit" -o "$out/${name}_$arch.o"
        objects="$objects $out/${name}_$arch.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -fms-extensions -fshort-wchar \
        -I. -Isource -Isource/cseries -Isource/math -Iport/mac \
        -idirafter port/linux/include -Iport/include/xdk -include "$base/mac_engine_semantics.h" \
        -c port/mac/engine_clock.c -o "$out/engine_clock_$arch.o"
    objects="$objects $out/engine_clock_$arch.o"
    for unit in $apply_units $vehicle_units $sky_units $native_sky_units $native_input_units $model_gpu_units $session_units $gpu_support_units source/bungie_net/network/transport_endpoint_winsock.c port/mac/engine_bitmaps.c port/mac/engine_services.c port/mac/engine_device_state.c port/mac/engine_arena.c port/mac/engine_game_runtime.c source/interface/hud_unit.c source/structures/structure_runtime_decals.c source/interface/hud.c tools/mac_hud_runtime_probe.c tools/mac_scenery_runtime_probe.c "$probe" tools/mac_player_queue_probe.c source/game/player_queues_new.c $menu_units port/mac/engine_world.c port/mac/native_scenario_gameplay.c port/mac/native_scenario_scripts.c port/mac/native_scenario_skies.c port/mac/engine_script_cache.c port/mac/native_actor_assets.c port/mac/native_decal_assets.c port/mac/native_physics_tags.c port/mac/native_antenna_tags.c port/mac/native_object_tags.c port/mac/native_bitmap_tags.c port/mac/native_light_tags.c port/mac/native_collision_tags.c port/mac/native_animation_tags.c port/mac/native_model_tags.c port/mac/native_model_buffers.c port/linux/game/first_person_legs.c port/linux/game/object_mesh.c source/render/render_objects.c source/objects/object_deleted_procs.c; do
        name=$(basename "$unit" .c)
        if [ "$unit" = port/mac/mac_render_timing.c ] || [ "$unit" = port/mac/mac_vertex_upload.c ] || [ "$unit" = port/mac/mac_index_decode_cache.c ] || [ "$unit" = port/mac/bsp_decode_cache.c ] || [ "$unit" = port/mac/mac_bone_gather.c ]; then
            "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
                -std=gnu89 -O2 $fp -Iport/mac -c "$unit" \
                -o "$out/${name}_$arch.o"
            objects="$objects $out/${name}_$arch.o"
            continue
        fi
        service_alias=
        unit_include=
        section_flags=
        case "$unit" in source/interface/player_ui.c|source/interface/ui_widget.c|source/interface/virtual_keyboard.c|source/interface/event_manager.c|source/memory/stack_memory_pool.c|source/interface/ui_widget_event_handler_functions.c|source/cache/cache_files.c|source/hs/hs.c|port/linux/game/menu_tags.c) section_flags="-ffunction-sections -fdata-sections" ;; esac
        if [ "$unit" = source/interface/ui_widget.c ]; then unit_include="-include port/mac/engine_wide_redirect.h"; fi
        if [ "$unit" = source/hs/hs.c ]; then unit_include="-include port/mac/mac_ctype_compat.h"; fi
        if [ "$unit" = source/cache/cache_files.c ]; then service_alias="-Dcache_file_tag_cache_contains=mac_raw_cache_file_tag_cache_contains -Dtag_loaded=mac_cache_files_tag_loaded -Dtag_iterator_new=mac_cache_files_tag_iterator_new -Dtag_iterator_next=mac_cache_files_tag_iterator_next -Dtag_get=mac_cache_files_tag_get -Dtag_get_name=mac_cache_files_tag_get_name -Dtag_get_group_tag=mac_cache_files_tag_get_group_tag -Dtag_index_is_group=mac_cache_files_tag_index_is_group -Dtag_reference_set=mac_cache_files_tag_reference_set -Dtag_groups_checksum=mac_cache_files_tag_groups_checksum"; fi
        if [ "$unit" = port/mac/engine_services.c ]; then service_alias="-Dcsmemcpy=mac_spawn_unused_memcpy -Dcsmemset=mac_spawn_unused_memset -Ddisplay_assert=mac_spawn_unused_assert -Derror=mac_spawn_unused_error -Dtag_block_get_element_with_size=mac_spawn_unused_tag_block"; fi
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp $apply_flags $section_flags $service_alias \
            -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files -Isource/memory \
            -Isource/cache -Isource/camera -Isource/bitmaps -Isource/main -Isource/editor -Isource/networking -Isource/ai -Isource/interface -Isource/rasterizer -Isource/rasterizer/xbox -Isource/text -Isource/cutscene -Isource/shaders -Isource/game -Isource/input -Isource/objects -Isource/items -Isource/units -Isource/models \
            -Isource/scenario -Isource/structures -Isource/render -Isource/sound -Isource/effects -Isource/shell -Isource/bink \
            -I"source/saved games" -I"$out" -I"$base" -Iport/mac -Iport/linux/game -idirafter port/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h -include port/mac/engine_xdk_prefix.h \
            -include "$base/mac_spawn_witness_names.h" -include port/include/halo_math.h \
            -include port/linux/include/halo_linux_source_fixups.h -include "$base/mac_engine_semantics.h" \
            $unit_include -c "$unit" -o "$out/${name}_$arch.o"
        objects="$objects $out/${name}_$arch.o"
    done
    if [ "${MAC_RUNTIME_VIEW:-0}" = 1 ];then
        for unit in port/mac/mac_window.c port/mac/mac_input.c port/mac/mac_bsp_renderer.c port/mac/mac_runtime_view.c;do
            if [ "$unit" = port/mac/mac_input.c ] && [ "${HALO_MAC_NATIVE_INPUT:-0}" = 1 ];then continue;fi
            name=$(basename "$unit" .c)
            "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
                -Wall -Wextra -Werror -Iport/mac -c "$unit" -o "$out/${name}_$arch.o"
            objects="$objects $out/${name}_$arch.o"
        done
        for unit in port/mac/mac_object_renderer.c port/mac/mac_object_render_probe.c port/mac/mac_live_object_view.c;do
            name=$(basename "$unit" .c)
            "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
                -std=gnu89 -O2 $fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
                -I. -Isource -Isource/cseries -Isource/math -Isource/tag_files -Isource/rasterizer \
                -Isource/objects -Isource/models -Iport/mac -idirafter port/linux/include -Iport/include/xdk \
                -include port/linux/include/halo_port_capacity.h -include port/mac/engine_xdk_prefix.h \
                -include port/include/halo_math.h -include "$base/mac_engine_semantics.h" \
                -c "$unit" -o "$out/${name}_$arch.o"
            objects="$objects $out/${name}_$arch.o"
        done
    fi
    printf "%s\n" $objects > "$out/objects_$arch.txt"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -Iport/mac -Wl,-dead_strip $objects \
        port/mac/mapped_file.c port/mac/retail_cache.c port/mac/retail_geometry.c port/mac/native_scenario_objects.c port/mac/native_scenario_blocks.c port/mac/retail_texture.c port/mac/retail_spawns.c port/mac/retail_skeleton.c port/mac/retail_object_spawn.c port/mac/native_tags.c port/mac/retail_collision_model.c port/mac/retail_collision.c port/mac/retail_animation.c port/mac/retail_animation_graph.c \
        $view_frameworks $input_frameworks -o "$out/object_runtime_$arch" > "$out/link_$arch.txt" 2>&1
done
run_arch=${MAC_RUN:-i386}
if [ "$run_arch" != none ];then
    "$out/object_runtime_$run_arch" "$@"
fi
