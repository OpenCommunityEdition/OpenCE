#!/bin/sh
# Link audit for the next genuine game_tick provider batch. It does not run the
# object-runtime probe and does not claim initialization or tick execution.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
base=build/mac-audit
out=${MAC_GAME_TICK_OUT:-build/mac-game-tick}
probe=${MAC_GAME_TICK_PROBE:-tools/mac_game_tick_probe.c}
mkdir -p "$out"
python tools/mac_material_effects_view.py "$out/mac_material_effects_native.c"
python tools/mac_dsound_view.py "$out/mac_dsound_native.c"
python tools/mac_dsound_globals_view.py source/sound/sound_dsound_xbox.c "$out/mac_sound_dsound_xbox.c"
python tools/mac_lruv_view.py "$out/mac_lruv_cache.c"
python tools/mac_voice_audio_view.py "$out/mac_voice_audio_native.c"
python tools/mac_game_state_storage_view.py "$out/mac_game_state_xbox.c"
python tools/mac_game_state_boundary.py "$base/mac_game_state_body.h"
python tools/mac_startup_globals_view.py "$out/mac_startup_globals.c"
python tools/mac_menu_files_view.py "$out/mac_menu_parser.c" "$out/mac_menu_xml.c"
python tools/mac_cache_vertex_view.py source/cache/cache_files_windows.c "$out/mac_cache_vertex_native.c"
python tools/mac_memory_diagnostics_view.py "$out/mac_memory_diagnostics.c"
python tools/mac_errors_view.py source/cseries/errors.c "$out/mac_errors.c"
python tools/mac_xgpu_texture_decode_view.py "$out/mac_xgpu_texture_decode_view.inc"
printf '#define __deprecated__(message)\n' > "$out/mac_expat_compat.h"
test -f port/mac/native_debug_globals.inc
sed -e 's/#include "platform.h"/#include "engine_wide.h"\
typedef int BOOL;\
#define TRUE 1\
#define FALSE 0/' \
    -e '/^#include <wchar.h>$/d' \
    -e 's/static __thread wchar_t \*next;/wchar_t **mac_next = mac_wide_tokenizer_state();\
#define next (*mac_next)/' \
    -e '/^wchar_t \*msvc_wcsdup/i\
#undef next\
' \
    -e 's/halo_linux_fopen/fopen/g' -e 's/halo_linux_freopen/freopen/g' \
    -e 's/halo_linux_remove/remove/g' port/linux/src/msvc_wide.c > "$out/mac_shared_wide.c"
cp port/mac/mac_bink_null.c "$out/mac_bink_null.c"
# The object runtime's established compilation manifest and generated headers
# are prerequisites; this script adds providers without editing that gate.
test -f build/mac-graphics-audit/objects_ppc.txt
test -f "$base/mac_engine_semantics.h"
test -f "$base/mac_spawn_witness_names.h"
providers='port/linux/game/coop_enemies.c port/linux/game/coop_scripts.c port/linux/game/coop_spectate.c port/linux/game/bmp_files.c port/linux/game/cache_file_formats.c port/linux/game/custom_edition_sounds.c port/linux/game/tag_validate.c port/linux/game/tag_schema_groups.c port/linux/game/tag_schema_objects.c port/linux/game/tag_schema_models.c port/linux/game/tag_schema_collision.c port/linux/game/tag_schema_render.c port/linux/game/tag_schema_effects.c port/linux/game/tag_schema_scenario.c port/linux/game/network_votekick.c source/interface/attract_mode.c $out/mac_errors.c source/bink/bink_playback.c source/interface/event_manager.c source/interface/hud_weapon.c source/interface/ui_widget_text_search_and_replace_functions.c source/hs/hs_library_external.c source/hs/object_lists.c source/input/input_abstraction.c source/interface/player_ui.c source/items/weapon_definitions.c source/interface/hud_draw.c source/networking/network_server_message_handler.c source/networking/network_game_manager.c source/networking/network_game_ui.c source/units/unit_definitions.c source/units/unit_scripting_commands.c source/interface/ui_widget.c source/interface/ui_widget_game_data_input_functions.c source/memory/array.c source/interface/motion_sensor.c port/third_party/stb_vorbis/stb_vorbis.c source/networking/network_messages.c source/interface/hud_sounds.c source/hs/hs.c source/hs/hs_compile.c source/hs/hs_globals_external.c source/game/game_engine_list.c source/game/game_engine_stub.c source/game/game_engine_ctf.c source/game/game_engine_oddball.c source/game/game_engine_king.c source/game/game_engine_race.c source/game/game_engine_slayer.c source/game/cheats.c source/interface/hud_nav_points.c source/interface/hud_messaging.c source/interface/hud_definitions.c source/interface/ui_widget_event_handler_functions.c source/memory/stack_memory_pool.c source/memory/circular_queue.c source/memory/data_encoding.c source/interface/virtual_keyboard.c source/cache/cache_files.c source/cache/xbox_sound_cache.c source/cache/physical_memory_map.c source/tag_files/files.c source/tag_files/files_windows.c source/camera/editor_flying_camera.c source/camera/flying_camera.c source/camera/orbiting_camera.c source/bungie_net/common/message_header.c source/bungie_net/network/transport_endpoint_winsock.c source/bungie_net/network/transport_endpoint_set_winsock.c source/memory/data_packet_groups.c source/memory/data_packets.c source/networking/network_connection.c source/rasterizer/rasterizer_cinematics.c source/rasterizer/rasterizer_lights.c source/rasterizer/xbox/rasterizer_xbox_hardware_geometry.c source/structures/structure_detail_objects.c source/structures/structure_lens_flares.c source/structures/structure_visibility.c source/tool/connected_geometry.c source/tool/error_geometry.c source/effects/weather_particle_systems.c source/models/models.c source/physics/collision_debug.c source/sound/sound_preferences.c port/linux/game/menu_tags.c port/linux/game/menu_functions.c port/linux/game/map_share.c port/linux/game/map_share_files.c port/linux/game/network_voice.c port/linux/game/powerup_render_bounds.c port/linux/game/hud_hires_tags.c port/linux/game/custom_edition_bitmaps.c port/linux/game/custom_edition_scripts.c port/linux/game/tiff_file.c port/linux/game/tag_validate_report.c port/linux/game/render_interpolation.c port/mac/engine_input_rumble.c port/mac/mac_input.c port/mac/mac_float_control.c'
status=0
for arch in ${MAC_ARCHS:-ppc}; do
    fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
    objects=
    failed=0
    : > "$out/compile_failures_$arch.txt"
    for file in $(cat build/mac-graphics-audit/objects_$arch.txt); do
        case "$file" in */material_effects_spawn_link_*|*/mac_material_effects_native_*|*hud_messaging_*|*/game_spawn_link_*|*/engine_arena_spawn_link_*|*/engine_services_spawn_link_*|*/engine_cache_files_*|*/engine_device_state_device_state_*|*/engine_config_config_*|*/engine_tags_spawn_link_*|*/engine_handles_*|*/engine_sockets_*|*/engine_clock_*|*/engine_common_spawn_link_*|*/engine_crt_spawn_link_*|*/engine_wide_wide_*|*/mac_shared_wide_wide_*|*/engine_format_*.o|*/models_spawn_link_*|*/collision_debug_spawn_link_*|*/render_interpolation_spawn_link_*|*/transport_endpoint_winsock_spawn_link_*|*/transport_endpoint_set_winsock_spawn_link_*|*/map_share_*.o|*/map_share_files_*.o|*/game_state_xbox_*|*/xbox_sound_cache_*|*/xbox_texture_cache_*|*/physical_memory_map_*|*/lruv_cache_*|*/engine_texture_cache_*|*/errors_spawn_link_*) continue ;; esac
        case "$file" in
            *mac_weapon_constructor_probe*|*/game_ppc.o|*/game_i386.o|*/mapped_file_*|*/retail_texture_*|*/retail_cache_*|*/retail_skeleton_*|*/retail_object_spawn_*|*/native_tags_*|*/retail_collision_model_*|*/retail_collision_*|*/retail_animation_*|*/retail_animation_graph_*|*/engine_world_*|*/engine_tags_spawn_link_*|*/bitmaps_spawn_link_*|*/native_actor_assets_*|*/native_bitmap_tags_*|*/native_object_tags_*|*/native_light_tags_*|*/native_collision_tags_*|*/native_animation_tags_*|*/native_animation_graph_*|*/native_model_tags_*|*/native_model_buffers_*|*/first_person_legs_*) continue ;;
        esac
        objects="$objects $file"
    done
    # Compile the same real-runtime units as the current object harness, but
    # keep the checked provider batch and its link result separate.
    for unit in "$out/mac_material_effects_native.c" port/mac/engine_services.c port/mac/engine_host_misc.c port/mac/engine_debug_globals.c port/mac/engine_device_state.c port/mac/mac_gpu_textures.c port/mac/mac_shader_source_cache.c port/mac/mac_bone_gather.c port/mac/mac_gl_visibility.c port/mac/mac_gl_program_cache.c port/mac/mac_render_timing.c port/mac/mac_vertex_upload.c port/mac/mac_index_decode_cache.c port/mac/bsp_decode_cache.c port/mac/engine_screen.c port/mac/mac_glsl120.c port/mac/mac_nv2a_vertex.c port/mac/mac_nv2a_pixel.c port/mac/mac_xgpu_text.c port/mac/engine_cache_resources.c port/mac/engine_arena.c port/mac/engine_handles.c port/mac/engine_sockets.c port/mac/engine_clock.c port/mac/engine_common.c port/mac/engine_legacy_shader_globals.c port/mac/engine_custom_edition.c port/mac/engine_cache_files.c port/mac/native_world_vertex_buffers.c port/mac/engine_session.c port/mac/engine_tags.c port/mac/engine_texture_cache.c port/mac/engine_texture_lifecycle.c port/mac/engine_synchronous_precache.c port/mac/engine_tag_compat.c port/mac/native_interface_assets.c port/mac/native_font_tags.c tools/mac_tag_compat_probe.c port/mac/engine_p2p_offline.c port/mac/engine_hud_hires_assets.c port/mac/native_antenna_tags.c port/mac/engine_config.c port/third_party/monocypher/monocypher.c tools/mac_texture_lifecycle_probe.c port/mac/engine_scenario_transition.c port/mac/engine_files.c port/mac/engine_xapi.c port/mac/engine_xnet_status.c port/mac/engine_xnet_compat.c port/mac/engine_winsock_init.c port/mac/mac_xnet_address.c "$out/mac_memory_diagnostics.c" port/mac/engine_shell.c port/mac/engine_console.c port/mac/engine_clipboard.c port/mac/engine_audio_devices.c port/mac/engine_display.c port/mac/engine_messages.c port/mac/engine_format.c "$out/mac_startup_globals.c" "$out/mac_menu_parser.c" "$out/mac_menu_xml.c" "$out/mac_cache_vertex_native.c" port/third_party/expat/xmlparse.c port/third_party/expat/xmlrole.c port/third_party/expat/xmltok.c source/sound/sound_environment_definitions.c "$out/mac_dsound_native.c" "$out/mac_sound_dsound_xbox.c" "$out/mac_lruv_cache.c" "$out/mac_voice_audio_native.c" port/mac/mac_audio_output.c port/mac/mac_voice_capture.c port/mac/mac_voice_dsp.c port/mac/mac_audio_pcm.c port/mac/engine_crt.c port/mac/engine_wide.c "$out/mac_shared_wide.c" "$out/mac_bink_null.c" port/mac/engine_physical_memory.c port/mac/engine_heap.c port/mac/mac_mbedtls_time.c port/mac/engine_memory_status.c port/mac/mac_host_memory.c port/mac/engine_game_runtime.c port/mac/engine_input.c port/mac/mac_window.c source/interface/hud_unit.c source/structures/structure_runtime_decals.c source/interface/hud.c "$probe" source/game/player_queues_new.c port/mac/engine_world.c port/mac/engine_bitmaps.c port/mac/native_actor_assets.c port/mac/native_physics_tags.c port/mac/native_environment_assets.c port/mac/native_effect_assets.c port/mac/native_decal_assets.c port/mac/native_object_tags.c port/mac/native_bitmap_tags.c port/mac/native_light_tags.c port/mac/native_collision_tags.c port/mac/native_animation_tags.c port/mac/native_model_tags.c port/mac/native_model_buffers.c port/mac/native_scenario_objects.c port/mac/native_scenario_blocks.c port/mac/native_scenario_gameplay.c port/mac/native_scenario_scripts.c port/mac/native_scenario_skies.c port/mac/engine_script_cache.c port/linux/game/first_person_legs.c port/linux/game/object_mesh.c source/render/render_objects.c source/objects/object_deleted_procs.c port/mac/mac_ctype.c $providers "source/saved games/saved_game_files.c" "source/saved games/playlist_profile.c" "$out/mac_game_state_xbox.c"; do
        name=$(basename "$unit" .c)
        if [ "$unit" = port/mac/mac_render_timing.c ] || [ "$unit" = port/mac/mac_vertex_upload.c ] || [ "$unit" = port/mac/mac_index_decode_cache.c ] || [ "$unit" = port/mac/bsp_decode_cache.c ] || [ "$unit" = port/mac/mac_bone_gather.c ]; then
            "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
                -std=gnu89 -O2 $fp -Iport/mac -c "$unit" \
                -o "$out/${name}_$arch.o"
            objects="$objects $out/${name}_$arch.o"
            continue
        fi
        if [ "$unit" = port/mac/engine_handles.c ] || [ "$unit" = port/mac/engine_sockets.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
                -D__int64='long long' -D__fastcall= -fshort-wchar \
                -ffunction-sections -fdata-sections -Iport/mac -Iport/include/xdk -Iport/linux/include -c "$unit" \
                -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then
                objects="$objects $out/${name}_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"
                failed=$((failed+1))
            fi
            continue
        fi
        if [ "$unit" = port/mac/engine_p2p_offline.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
                -ffunction-sections -fdata-sections -Iport/linux/src -Iport/third_party/monocypher \
                -c "$unit" -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then
                objects="$objects $out/${name}_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1))
            fi
            continue
        fi
        if [ "$unit" = port/mac/engine_config.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
                -ffunction-sections -fdata-sections -Iport/mac -Iport/linux/src \
                -c "$unit" -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then
                objects="$objects $out/${name}_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1))
            fi
            continue
        fi
        if [ "$unit" = port/third_party/monocypher/monocypher.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu99 -O2 $fp \
                -ffunction-sections -fdata-sections -Iport/third_party/monocypher \
                -c "$unit" -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then
                objects="$objects $out/${name}_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1))
            fi
            continue
        fi
        if [ "$unit" = port/mac/mac_input.c ] || [ "$unit" = port/mac/mac_window.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
                -ffunction-sections -fdata-sections -Iport/mac -c "$unit" -o "$out/${name}_$arch.o" \
                > "$out/${name}_$arch.txt" 2>&1; then
                objects="$objects $out/${name}_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1))
            fi
            continue
        fi
        if [ "$unit" = port/mac/mapped_file.c ] || [ "$unit" = port/mac/retail_cache.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
                -ffunction-sections -fdata-sections -Iport/mac -c "$unit" -o "$out/${name}_$arch.o" \
                > "$out/${name}_$arch.txt" 2>&1; then
                objects="$objects $out/${name}_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1))
            fi
            continue
        fi
        if [ "$unit" = port/mac/engine_files.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
                -ffunction-sections -fdata-sections -I. -Isource -Isource/cseries \
                -Isource/math -Iport/mac -Iport/include/xdk -Iport/linux/game \
                -idirafter port/linux/include -c "$unit" \
                -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then
                objects="$objects $out/${name}_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"
                failed=$((failed+1))
            fi
            continue
        fi
        if [ "$unit" = port/mac/engine_crt.c ] || [ "$unit" = port/mac/engine_wide.c ] || [ "$unit" = "$out/mac_shared_wide.c" ] || [ "$unit" = port/mac/engine_heap.c ] || [ "$unit" = port/mac/mac_audio_output.c ] || [ "$unit" = port/mac/mac_voice_capture.c ] || [ "$unit" = port/mac/mac_voice_dsp.c ] || [ "$unit" = port/mac/mac_audio_pcm.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -fshort-wchar -Iport/mac -Iport/linux/include -c "$unit" -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then objects="$objects $out/${name}_$arch.o"; else echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1)); fi
            continue
        fi
        if [ "$unit" = port/mac/engine_physical_memory.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files -Isource/memory -Isource/input -Isource/game -Isource/objects -Iport/mac -Iport/linux/game -Iport/linux/include -Iport/include/xdk -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h -include port/mac/engine_xdk_prefix.h -include port/include/halo_math.h -include "$base/mac_engine_semantics.h" -c "$unit" -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then objects="$objects $out/${name}_$arch.o"; else echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1)); fi
            continue
        fi
        if [ "$unit" = port/mac/native_scenario_gameplay.c ] || [ "$unit" = port/mac/native_scenario_scripts.c ] || [ "$unit" = port/mac/native_scenario_skies.c ] || [ "$unit" = port/mac/engine_script_cache.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
                -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv -ffunction-sections -fdata-sections \
                -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files \
                -Isource/scenario -Isource/structures -Isource/memory -Isource/game -Isource/input \
                -Isource/objects -Iport/mac -Iport/linux/game -Iport/linux/include -Iport/include/xdk \
                -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
                -include port/mac/engine_prefix.h -include port/include/halo_math.h \
                -include "$base/mac_engine_semantics.h" -c "$unit" -o "$out/${name}_$arch.o" \
                > "$out/${name}_$arch.txt" 2>&1; then
                objects="$objects $out/${name}_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"
                failed=$((failed+1))
            fi
            continue
        fi
        if [ "$unit" = port/mac/engine_xapi.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -ffunction-sections -fdata-sections -DMBEDTLS_CONFIG_FILE='"mac_mbedtls_config.h"' -I. -Isource -Isource/cseries -Isource/math -Iport/mac -Iport/include/xdk -Iport/third_party/mbedtls/include -idirafter port/linux/include -c "$unit" -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then objects="$objects $out/${name}_$arch.o"; else echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1)); fi
            continue
        fi
        if [ "$unit" = port/mac/mac_mbedtls_time.c ]; then
            # The whole of mbedtls and the whole of miniupnpc, for internet
            # play's DTLS and its port mapping. They hang off this unit
            # because this unit is the clock mbedtls asks for once
            # mac_mbedtls_config.h sets MBEDTLS_PLATFORM_MS_TIME_ALT: the
            # two belong together, and the library does not link without it.
            #
            # Every mbedtls file is built with the same MBEDTLS_CONFIG_FILE.
            # Its options decide structure layouts, so a library built half
            # one way and half the other does not fail at the link; it
            # misbehaves later, somewhere else. port/mac/engine_xapi.c,
            # which reads mbedtls headers, takes the same define above.
            #
            # Both are globbed rather than listed, as tools/linux_build.py
            # globs them, so that a vendored update brings its new files
            # with it. AGENTS.md keeps the vendored copies unchanged; all
            # the Mac needs is in mac_mbedtls_config.h.
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu99 -O2 $fp \
                -DMBEDTLS_CONFIG_FILE='"mac_mbedtls_config.h"' -ffunction-sections -fdata-sections \
                -Iport/mac -Iport/third_party/mbedtls/include \
                -c "$unit" -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then
                objects="$objects $out/${name}_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1))
            fi
            for mbedtls_source in port/third_party/mbedtls/library/*.c; do
                mbedtls_name=$(basename "$mbedtls_source" .c)
                if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu99 -O2 $fp \
                    -DMBEDTLS_CONFIG_FILE='"mac_mbedtls_config.h"' -ffunction-sections -fdata-sections \
                    -Iport/mac -Iport/third_party/mbedtls/include -Iport/third_party/mbedtls/library \
                    -c "$mbedtls_source" -o "$out/mbedtls_${mbedtls_name}_$arch.o" \
                    > "$out/mbedtls_${mbedtls_name}_$arch.txt" 2>&1; then
                    objects="$objects $out/mbedtls_${mbedtls_name}_$arch.o"
                else
                    echo "$mbedtls_source" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1))
                fi
            done
            for upnp_source in port/third_party/miniupnpc/src/*.c; do
                upnp_name=$(basename "$upnp_source" .c)
                if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu99 -O2 $fp \
                    -DMINIUPNP_STATICLIB -DMINIUPNPC_SET_SOCKET_TIMEOUT -DMINIUPNPC_GET_SRC_ADDR \
                    -ffunction-sections -fdata-sections -Iport/third_party/miniupnpc/include \
                    -c "$upnp_source" -o "$out/upnp_${upnp_name}_$arch.o" \
                    > "$out/upnp_${upnp_name}_$arch.txt" 2>&1; then
                    objects="$objects $out/upnp_${upnp_name}_$arch.o"
                else
                    echo "$upnp_source" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1))
                fi
            done
            continue
        fi
        if [ "$unit" = port/mac/native_scenario_objects.c ] || [ "$unit" = port/mac/native_scenario_blocks.c ] || [ "$unit" = port/mac/native_scenario_gameplay.c ] || [ "$unit" = port/mac/native_scenario_scripts.c ] || [ "$unit" = port/mac/engine_script_cache.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -ffunction-sections -fdata-sections -Iport/mac -Isource -c "$unit" -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then objects="$objects $out/${name}_$arch.o"; else echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1)); fi
            continue
        fi
        if [ "$unit" = port/mac/mac_host_memory.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
                -ffunction-sections -fdata-sections -Iport/mac -c "$unit" \
                -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then
                objects="$objects $out/${name}_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"
                failed=$((failed+1))
            fi
            continue
        fi
        if [ "$unit" = port/mac/engine_clock.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
                -D__int64='long long' -D__fastcall= -fshort-wchar \
                -ffunction-sections -fdata-sections -I. -Isource -Isource/cseries \
                -Iport/mac -Iport/include/xdk -Iport/linux/include -c "$unit" \
                -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then
                objects="$objects $out/${name}_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"
                failed=$((failed+1))
            fi
            continue
        fi
        if [ "$unit" = port/mac/engine_host_misc.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -ffunction-sections -fdata-sections -c "$unit" -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then objects="$objects $out/${name}_$arch.o"; else echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1)); fi
            continue
        fi
        if [ "$unit" = port/mac/engine_shell.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -Iport/mac -c "$unit" -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then objects="$objects $out/${name}_$arch.o"; else echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1)); fi
            continue
        fi
        if [ "$unit" = port/mac/engine_format.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -Iport/mac -c "$unit" -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then objects="$objects $out/${name}_$arch.o"; else echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1)); fi
            continue
        fi
        if [ "$unit" = "$out/mac_menu_parser.c" ] || [ "$unit" = "$out/mac_menu_xml.c" ]; then
            compat=; if [ "$unit" = "$out/mac_menu_parser.c" ]; then compat="-include $out/mac_expat_compat.h"; fi
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp $compat -I. -Iport/linux/src -Iport/third_party/expat -c "$unit" -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then objects="$objects $out/${name}_$arch.o"; else echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1)); fi
            continue
        fi
        case "$unit" in port/third_party/expat/xmlparse.c|port/third_party/expat/xmlrole.c|port/third_party/expat/xmltok.c)
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu99 -O2 $fp -include "$out/mac_expat_compat.h" -Iport/third_party/expat -c "$unit" -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then objects="$objects $out/${name}_$arch.o"; else echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1)); fi
            continue
            ;;
        esac
        if [ "$unit" = port/mac/engine_xnet_status.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
                -ffunction-sections -fdata-sections -c "$unit" -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then
                objects="$objects $out/${name}_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1))
            fi
            continue
        fi
        if [ "$unit" = port/mac/engine_xnet_compat.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
                -Dfast_ftol=engine_xnet_compat_unused_fast_ftol -fms-extensions -fshort-wchar \
                -ffunction-sections -fdata-sections -I. -Isource -Isource/cseries -Isource/math \
                -Iport/mac -Iport/include/xdk -idirafter port/linux/include \
                -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
                -include port/mac/engine_xdk_prefix.h -c "$unit" -o "$out/${name}_$arch.o" \
                > "$out/${name}_$arch.txt" 2>&1; then
                objects="$objects $out/${name}_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1))
            fi
            continue
        fi
        if [ "$unit" = port/mac/engine_winsock_init.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
                -Dfast_ftol=engine_winsock_init_unused_fast_ftol -fms-extensions -fshort-wchar \
                -ffunction-sections -fdata-sections -I. -Isource -Isource/cseries -Isource/math \
                -Iport/mac -Iport/include/xdk -idirafter port/linux/include \
                -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
                -include port/mac/engine_xdk_prefix.h -c "$unit" -o "$out/${name}_$arch.o" \
                > "$out/${name}_$arch.txt" 2>&1; then
                objects="$objects $out/${name}_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1))
            fi
            continue
        fi
        if [ "$unit" = port/mac/mac_xnet_address.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
                -Wall -Wextra -Werror -Iport/mac -ffunction-sections -fdata-sections \
                -c "$unit" -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then
                objects="$objects $out/${name}_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1))
            fi
            continue
        fi
        if [ "$unit" = "$out/mac_memory_diagnostics.c" ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
                -Dfast_ftol=mac_memory_diag_unused_fast_ftol -fms-extensions -fshort-wchar \
                -ffunction-sections -fdata-sections -I. -Isource -Isource/cseries -Isource/math \
                -Iport/mac -Iport/include/xdk -idirafter port/linux/include \
                -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
                -include port/mac/engine_xdk_prefix.h -c "$unit" -o "$out/${name}_$arch.o" \
                > "$out/${name}_$arch.txt" 2>&1; then
                objects="$objects $out/${name}_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1))
            fi
            continue
        fi
        if [ "$unit" = "$out/mac_voice_audio_native.c" ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
                -Dfast_ftol=mac_game_voice_unused_fast_ftol -fms-extensions -fshort-wchar \
                -fno-strict-aliasing -fwrapv -ffunction-sections -fdata-sections \
                -I. -Isource -Isource/cseries -Isource/math -Iport/mac -Iport/linux/src -Iport/linux/include \
                -Iport/include/xdk -Iport/third_party/opus/include -idirafter port/linux/include \
                -include port/linux/include/halo_port_limits.h -include port/mac/engine_xdk_prefix.h \
                -c "$unit" -o "$out/mac_voice_audio_$arch.o" > "$out/mac_voice_audio_$arch.txt" 2>&1; then
                objects="$objects $out/mac_voice_audio_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1))
            fi
            for opus_source in $(cat port/third_party/opus/sources.txt); do
                opus_name=$(basename "$opus_source" .c)
                if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu99 -O2 $fp \
                    -DOPUS_BUILD -DVAR_ARRAYS -DHAVE_LRINTF -DHAVE_LRINT \
                    -ffunction-sections -fdata-sections -Iport/third_party/opus/include \
                    -Iport/third_party/opus/celt -Iport/third_party/opus/silk -Iport/third_party/opus/silk/float \
                    -c "port/third_party/opus/$opus_source" -o "$out/opus_${opus_name}_$arch.o" \
                    > "$out/opus_${opus_name}_$arch.txt" 2>&1; then
                    objects="$objects $out/opus_${opus_name}_$arch.o"
                else
                    echo "port/third_party/opus/$opus_source" >> "$out/compile_failures_$arch.txt"; failed=$((failed+1))
                fi
            done
            continue
        fi
        service_alias=
        unit_include=
        if [ "$unit" = "$out/mac_dsound_native.c" ]; then service_alias=-DRESAMPLER_SCALAR; fi
        if [ "$unit" = port/mac/engine_input.c ]; then service_alias=-DMAC_INPUT_NO_CARBON; fi
        if [ "$unit" = port/mac/engine_clipboard.c ] || [ "$unit" = port/mac/engine_audio_devices.c ] || [ "$unit" = port/mac/engine_display.c ] || [ "$unit" = port/mac/engine_messages.c ]; then
            if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
                -ffunction-sections -fdata-sections -Iport/mac -c "$unit" -o "$out/${name}_$arch.o" \
                > "$out/${name}_$arch.txt" 2>&1; then
                objects="$objects $out/${name}_$arch.o"
            else
                echo "$unit" >> "$out/compile_failures_$arch.txt"
                failed=$((failed+1))
            fi
            continue
        fi
        if [ "$unit" = "$out/mac_errors.c" ]; then unit_include="-include port/mac/engine_atomic_file.h"; fi
        if [ "$unit" = source/interface/hud_draw.c ]; then service_alias=-D__INTPTR_TYPE__=long; fi
        if [ "$unit" = source/interface/ui_widget.c ]; then unit_include="-include port/mac/engine_wide_redirect.h"; fi
        if [ "$unit" = source/hs/hs.c ]; then unit_include='-include port/mac/mac_ctype_compat.h'; fi
        if [ "$unit" = source/game/game_engine_king.c ]; then unit_include='-include port/mac/mac_game_engine_king_compat.h'; fi
        if [ "$unit" = source/cache/cache_files.c ]; then service_alias='-Dcache_file_tag_cache_contains=mac_raw_cache_file_tag_cache_contains -Dtag_loaded=mac_cache_files_tag_loaded -Dtag_iterator_new=mac_cache_files_tag_iterator_new -Dtag_iterator_next=mac_cache_files_tag_iterator_next -Dtag_get=mac_cache_files_tag_get -Dtag_get_name=mac_cache_files_tag_get_name -Dtag_get_group_tag=mac_cache_files_tag_get_group_tag -Dtag_index_is_group=mac_cache_files_tag_index_is_group -Dtag_reference_set=mac_cache_files_tag_reference_set -Dtag_groups_checksum=mac_cache_files_tag_groups_checksum'; fi
        if [ "$unit" = port/mac/engine_services.c ]; then service_alias="-Dcsmemcpy=mac_spawn_unused_memcpy -Dcsmemset=mac_spawn_unused_memset -Ddisplay_assert=mac_spawn_unused_assert -Derror=mac_spawn_unused_error -Dtag_block_get_element_with_size=mac_spawn_unused_tag_block"; fi
        if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp $service_alias -DHALO_MAC \
            -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv -ffunction-sections -fdata-sections \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files -Isource/memory \
            -Isource/cache -Isource/camera -Isource/bitmaps -Isource/main -Isource/editor -Isource/networking -Isource/ai -Isource/interface -Isource/rasterizer -Isource/rasterizer/xbox -Isource/text -Isource/cutscene -Isource/shaders -Isource/game -Isource/input -Isource/objects -Isource/items -Isource/units -Isource/models \
            -Isource/scenario -Isource/structures -Isource/render -Isource/sound -Isource/effects -Isource/shell -Isource/bink \
            -I"source/saved games" -I"$out" -I"$base" -Iport/mac -Iport/linux/game -Iport/linux/src -Iport/third_party/stb_vorbis -idirafter port/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h -include port/mac/engine_xdk_prefix.h \
            -include "$base/mac_spawn_witness_names.h" -include port/include/halo_math.h \
            -include port/linux/include/halo_linux_source_fixups.h -include "$base/mac_engine_semantics.h" \
            $unit_include \
            -c "$unit" -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then
            objects="$objects $out/${name}_$arch.o"
        else
            echo "$unit" >> "$out/compile_failures_$arch.txt"
            failed=$((failed+1))
        fi
    done
    printf '%s\n' $objects > "$out/engine_objects_$arch.txt"
    if [ -f tools/mac_wide_import_check.sh ]; then
        sh tools/mac_wide_import_check.sh "$out/engine_objects_$arch.txt"
    fi
    if "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -Iport/mac -Wl,-dead_strip $objects \
        -framework AudioUnit -framework AudioToolbox -framework CoreAudio -framework CoreFoundation -framework Carbon -framework ApplicationServices -framework AGL -framework OpenGL \
        port/mac/mapped_file.c port/mac/retail_cache.c port/mac/retail_geometry.c port/mac/retail_texture.c port/mac/retail_spawns.c port/mac/retail_skeleton.c port/mac/retail_object_spawn.c port/mac/native_tags.c port/mac/retail_collision_model.c port/mac/retail_collision.c port/mac/retail_animation.c port/mac/retail_animation_graph.c \
        -o "$out/game_tick_link_$arch" > "$out/link_$arch.txt" 2>&1; then
        echo "$arch: provider batch linked; $failed source compile failures; not executed"
    else
        echo "$arch: provider batch link incomplete; $failed source compile failures; see $out/link_$arch.txt"
        status=1
    fi
    if [ "$failed" -ne 0 ]; then cat "$out/compile_failures_$arch.txt"; status=1; fi
done
exit "$status"
