#!/bin/sh
# Compile the original rasterizer/render graph into the two-cycle retail
# session probe. The default session checker remains CPU/headless-only.
set -eu

if [ "$#" -ne 3 ]; then
    echo "usage: $0 bloodgulch.map bitmaps.map sounds.map" >&2
    exit 2
fi

cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
base=${MAC_SESSION_MANIFEST:-build/mac-game-tick-session-final38}
out=${MAC_RENDERED_SESSION_OUT:-build/mac-rendered-session}
audit=${MAC_RENDER_AUDIT_BASE:-build/mac-audit}
mkdir -p "$out"

# Reuse the source-checked original main_game_render view and its reachable
# original render units, but start from the latest complete session manifest.
# Incremental mode reuses that graph and the native objects already listed in
# this output directory, recompiling only the whitespace-separated basenames
# named by MAC_RENDERED_SESSION_RECOMPILE (for example engine_device_state).
if [ "${MAC_RENDERED_SESSION_INCREMENTAL:-0}" = 1 ]; then
    for arch in ${MAC_ARCHS:-ppc i386}; do
        test -f "$out/render-graph/engine_objects_$arch.txt"
    done
else
    MAC_RENDER_MANIFEST=$base MAC_RENDER_OUT="$out/render-graph" \
        MAC_RENDER_AUDIT_BASE=$audit MAC_ARCHS="${MAC_ARCHS:-ppc i386}" \
        sh tools/mac_main_game_render_checks.sh
fi

# port: the graph's prebuilt objects (build/mac-game-tick, mac-audit, mac-graphics-audit) are not rebuilt by anything
# when a source changes. Every unit under source/ and port/linux/game is compiled from the current tree, compared with
# its prebuilt object and, where the code differs, linked in its place (tools/mac_graph_refresh.sh). MAC_GRAPH_REFRESH=off
# links the prebuilt objects as they are.
graph_list() {
    if [ "${MAC_GRAPH_REFRESH:-on}" = off ]; then
        echo "$out/render-graph/engine_objects_$1.txt"
    else
        echo "$out/render-graph/engine_objects_refreshed_$1.txt"
    fi
}
if [ "${MAC_GRAPH_REFRESH:-on}" != off ]; then
    MAC_ARCHS="${MAC_ARCHS:-ppc i386}" sh tools/mac_graph_refresh.sh "$out" "$audit"
fi

default_pixels_source=${MAC_RASTERIZER_DEFAULT_SOURCE:-build/mac-audit/mac_rasterizer_xbox_default_pixels.c}
if [ -f "$default_pixels_source" ]; then
    cp "$default_pixels_source" "$out/mac_rasterizer_xbox_default_pixels.c"
else
    python tools/mac_rasterizer_default_pixels_view.py "$out/mac_rasterizer_xbox_default_pixels.c"
fi
python tools/mac_device_state_view.py \
    "$out/mac_device_state_body.inc" "$out/mac_device_state_defaults.inc"
python tools/mac_dsound_view.py "$out/mac_dsound_native.c"
# port: errors.c with HALO_MAC_ENGINE_LOG, so the engine can say on stderr
# why it refused something (network_event's lines are all error() calls)
python tools/mac_errors_view.py source/cseries/errors.c "$out/mac_errors.c"
# port: main.c's view again, with the menus' request reader appended (the graph's is excluded below)
python tools/mac_main_game_render_view.py "$out/mac_main_game_render.c"
# port: the menus' XML again with the Mac's own screens (the pause menu) after them, and the parser for the Mac below
python tools/mac_menu_files_view.py --mac "$out/mac_menu_parser_view.c" "$out/mac_menu_xml.c"
# port: the high-res HUD's textures (port/assets/hud_mac), drawn by mac_hud_hires.c in place of the engine's own empty set
python tools/mac_hud_assets_view.py "$out/mac_hud_assets.c"
python tools/mac_xgpu_texture_decode_view.py "$out/mac_xgpu_texture_decode_view.inc"
python tools/mac_cache_files_native_view.py source/cache/cache_files.c \
    "$out/mac_cache_files_native.c"
# port: the engine's own texture cache, for Xbox maps (tools/mac_xbox_texture_cache_view.py)
python tools/mac_xbox_texture_cache_view.py source/cache/xbox_texture_cache.c \
    "$out/mac_xbox_texture_cache.c"

python tools/mac_material_effects_view.py "$out/mac_material_effects_native.c"
# port: the game state's checkpoint kept in memory (replaces the graph's mac_game_state_xbox)
python tools/mac_game_state_storage_view.py "$out/mac_game_state_xbox.c"
tag_diag_unit=
if [ "${MAC_TAG_BOUNDS_DIAGNOSTIC:-0}" = 1 ]; then
    tag_diag_unit="$out/mac_tag_groups_diag.c"
    python tools/mac_tag_groups_diag_view.py "$tag_diag_unit"
fi

compile_engine_unit() {
    arch=$1
    source=$2
    object=$3
    extra=${4:-}
    last=
    # port: ui_widget.c's wide-string redirect (as in mac_game_tick_checks.sh)
    if [ "$source" = source/interface/ui_widget.c ]; then last="-include port/mac/engine_wide_redirect.h"; fi
    fp=
    if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    # shellcheck disable=SC2086
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -DHALO_MAC -fms-extensions -fshort-wchar \
        -fno-strict-aliasing -fwrapv -ffunction-sections -fdata-sections \
        -I"$out" -I"$out/render-graph" -I"$audit" -I. \
        -Isource -Isource/cseries -Isource/math -Isource/physics \
        -Isource/tag_files -Isource/memory -Isource/cache -Isource/camera \
        -Isource/bitmaps -Isource/main -Isource/editor -Isource/networking \
        -Isource/ai -Isource/interface -Isource/rasterizer \
        -Isource/rasterizer/xbox -Isource/text -Isource/cutscene \
        -Isource/shaders -Isource/game -Isource/input -Isource/objects \
        -Isource/items -Isource/units -Isource/models -Isource/scenario \
        -Isource/structures -Isource/render -Isource/sound -Isource/effects \
        -Isource/shell -Isource/bink -I"source/saved games" \
        -Iport/mac -Iport/linux/game -Iport/linux/src \
        -Iport/third_party/stb_vorbis -idirafter port/linux/include \
        -Iport/include/xdk \
        -include port/linux/include/halo_port_limits.h \
        -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_xdk_prefix.h \
        -include "$audit/mac_spawn_witness_names.h" \
        -include port/include/halo_math.h \
        -include port/linux/include/halo_linux_source_fixups.h \
        -include "$audit/mac_engine_semantics.h" $last \
        $extra \
        -c "$source" -o "$object" > "$object.txt" 2>&1
}

compile_native_helper() {
    arch=$1
    source=$2
    object=$3
    fp=
    if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    # Retain the renderer-link checker's exact platform-helper flags. These
    # sources include their headers from port/mac and must not receive the
    # game's forced XDK prefix or its include aliases.
    # shellcheck disable=SC2086
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -Iport/mac \
        -c "$source" -o "$object" > "$object.txt" 2>&1
}

for arch in ${MAC_ARCHS:-ppc i386}; do
    fp=
    if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    objects=
    recompile=" ${MAC_RENDERED_SESSION_RECOMPILE:-} "
    provenance="$out/source_provenance_$arch.txt"
    if [ "${MAC_RENDERED_SESSION_INCREMENTAL:-0}" = 1 ] && [ -f "$provenance" ]; then
        stamp=$(date '+%Y%m%dT%H%M%S')
        cp "$provenance" "$out/source_provenance_${arch}_before_incremental_${stamp}.txt"
    fi
    # port: MAC_TUNE_UNITS="source/memory/data.c ..." compiles those game units again for the
    # G4 (-mtune=7450: instruction scheduling only, the same results) and links them in place
    # of the graph's objects
    tuned=" "
    if [ "$arch" = ppc ] && [ -n "${MAC_TUNE_UNITS:-}" ]; then
        mkdir -p "$out/tuned"
        for tune_unit in $MAC_TUNE_UNITS; do
            tune_name=$(basename "$tune_unit" .c)
            rm -f "$out/tuned/${tune_name}_ppc.o"
            compile_engine_unit ppc "$tune_unit" "$out/tuned/${tune_name}_ppc.o" -mtune=7450 || true
            if [ -f "$out/tuned/${tune_name}_ppc.o" ]; then tuned="$tuned$tune_name "; fi
        done
    fi
    for obj in $(cat "$(graph_list "$arch")"); do
        case "$obj" in */*_ppc.o)
            tune_base=$(basename "$obj" _ppc.o)
            tune_base=${tune_base%_spawn_link}
            case "$tuned" in *" $tune_base "*)
                objects="$objects $out/tuned/${tune_base}_ppc.o"; continue ;;
            esac ;;
        esac
        if [ -n "$tag_diag_unit" ]; then
            case "$obj" in */tag_groups_spawn_link_$arch.o) continue ;; esac
        fi
        # A settings table rebuilt into this output (tools/mac_config_checks.sh
        # with MAC_CHECK_DIR=$out/config) replaces the one the graph lists.
        case "$obj" in */mac_shared_config_config_$arch.o)
            if [ -f "$out/config/mac_shared_config_config_$arch.o" ]; then
                objects="$objects $out/config/mac_shared_config_config_$arch.o"; continue
            fi ;;
        esac
        # port: engine_config.c (halo_interpolation_enabled's frame-time test) comes from the same rebuild
        case "$obj" in */engine_config_$arch.o)
            if [ -f "$out/config/engine_config_config_$arch.o" ]; then
                objects="$objects $out/config/engine_config_config_$arch.o"; continue
            fi ;;
        esac
        case "$obj" in
            */material_effects_spawn_link_$arch.o|*/mac_material_effects_native_$arch.o|\
            */shared_d3d_resources_cpu_resources_$arch.o|*/mac_engine_session_probe_$arch.o|*/mac_tag_compat_probe_$arch.o|*/engine_device_state_$arch.o|\
            */engine_world_$arch.o|*/engine_game_variant_$arch.o|*/engine_session_$arch.o|*/engine_cache_files_$arch.o|*/engine_tags_$arch.o|*/cache_files_$arch.o|\
            */mac_window_$arch.o|*/mac_audio_output_$arch.o|*/mac_input_$arch.o|*/engine_messages_$arch.o|*/mac_gl_visibility_$arch.o|*/mac_gl_program_cache_$arch.o|*/mac_vertex_upload_$arch.o|*/mac_index_decode_cache_$arch.o|*/bsp_decode_cache_$arch.o|*/mac_bone_gather_$arch.o|*/mac_nv2a_arb_$arch.o|*/mac_nv2a_arbfp_$arch.o|\
            */engine_device_resources_$arch.o|*/engine_device_factory_$arch.o|\
            */engine_texture_lifecycle_$arch.o|*/engine_cache_resources_$arch.o|*/engine_physical_memory_$arch.o|*/engine_tag_compat_$arch.o|*/engine_script_cache_$arch.o|*/xbox_texture_cache_$arch.o|\
            */engine_rasterizer_services_$arch.o|*/engine_text_text_$arch.o|*/native_object_tags_$arch.o|*/native_effect_assets_$arch.o|*/native_interface_assets_$arch.o|*/native_font_tags_$arch.o|*/native_interface_dependencies_$arch.o|\
            */engine_texture_cache_$arch.o|*/native_bitmap_tags_$arch.o|*/native_scenario_blocks_$arch.o|*/native_scenario_objects_$arch.o|*/native_scenario_gameplay_$arch.o|*/native_environment_assets_$arch.o|*/native_animation_tags_$arch.o|*/native_sky_tags_$arch.o|*/native_sky_assets_$arch.o|*/native_scenario_scripts_$arch.o|*/engine_input_$arch.o|*/engine_screen_$arch.o|*/engine_hud_hires_assets_$arch.o|*/menu_tags_$arch.o|*/menu_functions_$arch.o|*/mac_main_game_render_$arch.o|*/mac_menu_xml_$arch.o|*/mac_menu_parser_$arch.o|*/engine_render_services_$arch.o|*/engine_synchronous_precache_$arch.o|*/engine_handles_$arch.o|*/engine_display_$arch.o|\
            */native_resource_memory*_$arch.o|*/mac_glsl120_$arch.o|\
            */native_model_buffers_$arch.o|*/native_actor_assets_$arch.o|\
            */mac_nv2a_vertex_$arch.o|*/mac_nv2a_pixel_$arch.o|\
            */mac_xgpu_text_$arch.o|*/mac_dsound_native_$arch.o|\
            */rasterizer_xbox_$arch.o|*/models_$arch.o|\
            */rasterizer_spawn_link_$arch.o|*/rasterizer_public_$arch.o|*/render_objects_$arch.o|*/render_particles_$arch.o|\
            */rasterizer_detail_$arch.o|*/engine_files_$arch.o|*/hs_library_external_$arch.o|*/weapons_spawn_link_$arch.o|*/ui_widget_$arch.o|*/hud_messaging_$arch.o|*/engine_game_engine_spawn_link_$arch.o|\
            */mac_game_state_xbox_$arch.o|*/rasterizer_vertex_init_$arch.o|*/cache_files_$arch.o|*/scenario_spawn_link_$arch.o|*/engine_scenario_transition_$arch.o) continue ;;
        esac
        # port: the netcode is compiled from this checkout rather than taken
        # from the manifest. The wire's byte order (port/linux/include/halo_endian.h)
        # and the Mac's sockets changed after those objects were made, and a
        # mixture of the two would be neither.
        case "$obj" in
            */engine_sockets_$arch.o|*/engine_winsock_init_$arch.o|*/engine_p2p_offline_$arch.o|\
            */data_encoding_$arch.o|*/message_header_$arch.o|\
            */transport_endpoint_winsock_$arch.o|*/transport_endpoint_set_winsock_$arch.o|\
            */game_engine_slayer_$arch.o|*/game_engine_ctf_$arch.o|*/game_engine_oddball_$arch.o|\
            */game_engine_race_$arch.o|*/game_engine_king_$arch.o|\
            */network_voice_$arch.o|*/network_votekick_$arch.o|*/map_share_$arch.o|\
            */network_messages_$arch.o|*/mac_errors_$arch.o|\
            */engine_xnet_compat_$arch.o|*/ui_widget_event_handler_functions_$arch.o) continue ;;
        esac
        # port: and the spawn-link stand-ins for the netcode units, which the
        # real units now define (tools/mac_spawn_link_checks.sh made them when
        # nothing linked the netcode)
        case "$obj" in
            */network_game_globals_spawn_link_$arch.o|*/network_client_manager_spawn_link_$arch.o|\
            */network_server_manager_spawn_link_$arch.o|*/network_distributed_spawn_link_$arch.o|\
            */network_objects_spawn_link_$arch.o|*/network_damage_spawn_link_$arch.o|\
            */network_actors_spawn_link_$arch.o|*/network_coop_spawn_link_$arch.o) continue ;;
        esac
        objects="$objects $obj"
    done

    # Define the opt-in only for this probe object; ordinary headless builds
    # compile the same source without GPU-only dependencies.
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -DMAC_SESSION_GPU_RENDER -DHALO_MAC \
        -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
        -I"$out" -I"$out/render-graph" -I"$audit" -I. \
        -Isource -Isource/cseries -Isource/math -Isource/physics \
        -Isource/tag_files -Isource/memory -Isource/cache -Isource/camera \
        -Isource/bitmaps -Isource/main -Isource/editor -Isource/networking \
        -Isource/ai -Isource/interface -Isource/rasterizer \
        -Isource/rasterizer/xbox -Isource/text -Isource/cutscene \
        -Isource/shaders -Isource/game -Isource/input -Isource/objects \
        -Isource/items -Isource/units -Isource/models -Isource/scenario \
        -Isource/structures -Isource/render -Isource/sound -Isource/effects \
        -Isource/shell -Isource/bink -I"source/saved games" \
        -Iport/mac -Iport/linux/game -Iport/linux/src \
        -Iport/third_party/stb_vorbis -idirafter port/linux/include \
        -Iport/include/xdk \
        -include port/linux/include/halo_port_limits.h \
        -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_xdk_prefix.h \
        -include "$audit/mac_spawn_witness_names.h" \
        -include port/include/halo_math.h \
        -include port/linux/include/halo_linux_source_fixups.h \
        -include "$audit/mac_engine_semantics.h" \
        -c tools/mac_engine_session_probe.c \
        -o "$out/mac_engine_session_probe_gpu_$arch.o" \
        > "$out/mac_engine_session_probe_gpu_$arch.o.txt" 2>&1

    # Original Xbox rasterizer initialization and its genuine native services.
    units="$tag_diag_unit $out/mac_material_effects_native.c port/mac/native_world_vertex_buffers.c port/mac/engine_world.c port/mac/native_scenario_scripts.c port/mac/engine_input.c port/mac/engine_game_variant.c port/mac/native_interface_assets.c port/mac/native_font_tags.c port/mac/native_interface_dependencies.c port/mac/native_effect_assets.c port/mac/native_object_tags.c port/mac/native_scenario_blocks.c port/mac/native_scenario_objects.c port/mac/native_scenario_gameplay.c port/mac/native_environment_assets.c port/mac/native_animation_tags.c port/mac/native_sky_tags.c port/mac/native_sky_assets.c port/mac/shared_d3d_resources.c tools/mac_tag_compat_probe.c port/mac/engine_tags.c tools/mac_session_interactive.c source/rasterizer/rasterizer.c source/render/render_objects.c source/models/models.c source/render/render_particles.c port/linux/game/render_detail.c $out/mac_rasterizer_xbox_default_pixels.c source/rasterizer/xbox/rasterizer_xbox_vertex_shaders_initialize.c port/mac/engine_device_state.c port/mac/engine_device_factory.c port/mac/engine_device_resources.c port/mac/engine_rasterizer_services.c port/mac/native_bitmap_tags.c port/mac/engine_texture_cache.c port/mac/native_resource_memory.c port/mac/native_model_buffers.c port/mac/native_actor_assets.c port/mac/mac_gpu_textures.c port/mac/mac_shader_source_cache.c port/mac/mac_bone_gather.c port/mac/mac_nv2a_arb.c port/mac/mac_nv2a_arbfp.c port/mac/mac_vertex_upload.c port/mac/mac_index_decode_cache.c port/mac/bsp_decode_cache.c port/mac/mac_glsl120.c port/mac/mac_nv2a_vertex.c port/mac/mac_nv2a_pixel.c port/mac/mac_xgpu_text.c $out/mac_dsound_native.c port/mac/mapped_file.c port/mac/retail_cache.c port/mac/retail_geometry.c port/mac/retail_texture.c port/mac/retail_spawns.c port/mac/retail_skeleton.c port/mac/retail_object_spawn.c port/mac/native_tags.c port/mac/retail_collision_model.c port/mac/retail_collision.c port/mac/retail_animation.c port/mac/retail_animation_graph.c port/mac/engine_cache_files.c port/mac/mac_sound_convert.c port/mac/engine_session.c $out/mac_cache_files_native.c port/mac/engine_menu.c port/linux/game/menu_tags.c port/mac/engine_menu_art.c port/mac/engine_display.c $out/mac_main_game_render.c $out/mac_menu_xml.c source/scenario/scenario.c port/mac/engine_scenario_transition.c source/hs/hs_library_external.c port/mac/engine_game_engine.c source/interface/ui_widget.c source/items/weapons.c port/linux/src/texture_pack.c port/mac/engine_render_services.c port/mac/engine_synchronous_precache.c port/mac/engine_handles.c source/interface/hud_messaging.c port/mac/engine_texture_lifecycle.c port/mac/engine_cache_resources.c port/mac/engine_physical_memory.c port/mac/engine_tag_compat.c port/mac/engine_script_cache.c $out/mac_xbox_texture_cache.c port/mac/xbox_map.c port/mac/xbox_swap.c port/mac/xbox_header.c port/mac/xbox_geometry.c port/mac/xbox_blob.c port/linux/game/menu_functions.c $out/mac_game_state_xbox.c port/mac/engine_screen.c port/mac/xbox_tag_tables.c port/mac/xbox_tag_walk.c"
    # port: the netcode. The game's own networking, the port's distributed
    # netcode and the automated system link test (network_test.c), which
    # tools/mac_session_interactive.c drives as source/main/main.c's loop does.
    units="$units source/networking/network_game_globals.c source/networking/network_client_manager.c source/networking/network_server_manager.c source/networking/network_client_message_handler.c source/networking/network_game_preferences.c"
    units="$units source/bungie_net/common/message_header.c source/bungie_net/common/public_key_crypt.c source/bungie_net/common/64bit_math.c source/bungie_net/common/prime_numbers.c source/bungie_net/common/random_numbers.c source/bungie_net/network/transport_endpoint_winsock.c source/bungie_net/network/transport_endpoint_set_winsock.c"
    units="$units source/memory/data_encoding.c source/game/game_engine_slayer.c source/game/game_engine_ctf.c source/game/game_engine_oddball.c source/game/game_engine_race.c source/game/game_engine_king.c"
    units="$units port/linux/game/network_distributed.c port/linux/game/network_objects.c port/linux/game/network_damage.c port/linux/game/network_actors.c port/linux/game/network_coop.c port/linux/game/network_votekick.c port/linux/game/network_voice.c port/linux/game/network_test.c port/linux/game/map_share.c"
    # network_messages.c defines network_event, which is how the netcode says
    # what went wrong. The manifest's copy was built with that logging out, so
    # every network_event call anywhere in the netcode said nothing.
    units="$units source/networking/network_messages.c $out/mac_errors.c"
    units="$units port/mac/engine_winsock_init.c port/mac/engine_xnet_compat.c port/mac/engine_network_test.c"
    # port: the pause menu's LEAVE GAME in a network game (ui_widget_event_handler_functions.c)
    units="$units source/interface/ui_widget_event_handler_functions.c"
    # port: the menus' mouse pointer (menu_cursor.c, input.menu_cursor)
    units="$units port/linux/game/menu_cursor.c"
    source_units=
    recognized_units=" mac_engine_session_probe engine_text_gl "
    index=0
    for unit in $units; do
        name=$(basename "$unit" .c)
        if [ "$unit" = "$out/mac_rasterizer_xbox_default_pixels.c" ]; then
            name=rasterizer_xbox_default_pixels
        elif [ "$unit" = "$out/mac_dsound_native.c" ]; then
            name=mac_dsound_native
        elif [ "$unit" = "$out/mac_xbox_texture_cache.c" ]; then
            name=mac_xbox_texture_cache
        fi
        recognized_units="$recognized_units $name "
        obj="$out/${name}_gpu_$arch.o"
        if [ "${MAC_RENDERED_SESSION_INCREMENTAL:-0}" != 1 ] || \
            case "$recompile" in *" $name "*) true ;; *) false ;; esac; then
            case "$unit" in
              port/mac/mapped_file.c|port/mac/xbox_map.c|port/mac/xbox_swap.c|port/mac/xbox_header.c|port/mac/xbox_blob.c|port/mac/retail_cache.c|port/mac/retail_geometry.c|port/mac/xbox_tag_tables.c|port/mac/xbox_tag_walk.c|\
              port/mac/retail_texture.c|port/mac/retail_spawns.c|port/mac/retail_skeleton.c|\
              port/mac/retail_object_spawn.c|port/mac/native_tags.c|\
              port/mac/retail_collision_model.c|port/mac/retail_collision.c|\
              port/mac/retail_animation.c|port/mac/retail_animation_graph.c|\
              port/mac/mac_vertex_upload.c|port/mac/mac_bone_gather.c|port/mac/mac_nv2a_arb.c|port/mac/mac_index_decode_cache.c|port/mac/bsp_decode_cache.c|port/mac/engine_menu_art.c|port/mac/engine_display.c|port/linux/src/texture_pack.c|port/mac/engine_handles.c)
                compile_native_helper "$arch" "$unit" "$obj" ;;
              "$out/mac_dsound_native.c")
                compile_engine_unit "$arch" "$unit" "$obj" -DRESAMPLER_SCALAR
                ;;
              "$out/mac_errors.c")
                compile_engine_unit "$arch" "$unit" "$obj" \
                    "-include port/mac/engine_atomic_file.h"
                ;;
              source/networking/network_server_manager.c)
                # port: with DEBUG, which port/linux/port.json gives the game
                # sources on every other platform, the join token is the
                # game's fixed one; without it the token takes the build's
                # time, and a machine of another build is refused as having
                # a bad join token (a Linux host and a Mac client)
                compile_engine_unit "$arch" "$unit" "$obj" "-DDEBUG"
                ;;
              source/game/game_engine_king.c)
                # it repeats two of rasterizer_model_types.h's layout typedefs,
                # which C89 refuses (as tools/mac_game_tick_checks.sh has it)
                compile_engine_unit "$arch" "$unit" "$obj" \
                    "-include port/mac/mac_game_engine_king_compat.h"
                ;;
              *) compile_engine_unit "$arch" "$unit" "$obj" ;;
            esac
        elif [ ! -f "$obj" ]; then
            echo "incremental object is missing: $obj" >&2
            exit 1
        fi
        objects="$objects $obj"
        source_units="$source_units $unit"
    done

    # port: the menus' parser for the Mac (its view defines MENU_PLATFORM_MAC: the
    # elements of the platform mac), in place of the graph's
    name=mac_menu_parser_view
    recognized_units="$recognized_units $name "
    obj="$out/${name}_gpu_$arch.o"
    if [ "${MAC_RENDERED_SESSION_INCREMENTAL:-0}" != 1 ] || \
        case "$recompile" in *" $name "*) true ;; *) false ;; esac; then
        printf '#define __deprecated__(message)\n' > "$out/mac_expat_compat.h"
        "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
            -include "$out/mac_expat_compat.h" -I. -Iport/linux/src -Iport/third_party/expat \
            -c "$out/mac_menu_parser_view.c" -o "$obj" > "$obj.txt" 2>&1
    elif [ ! -f "$obj" ]; then
        echo "incremental object is missing: $obj" >&2
        exit 1
    fi
    objects="$objects $obj"
    source_units="$source_units $out/mac_menu_parser_view.c"

    # port: the socket adapter, the settings it reads and the stand-ins for
    # internet play. These include the Darwin headers under private names and
    # must not take the game's prefix or its include aliases.
    for unit in port/mac/engine_sockets.c port/linux/src/net_settings.c port/linux/src/net_fault.c port/mac/engine_p2p_offline.c port/linux/src/menu_pc_art.c; do
        name=$(basename "$unit" .c)
        recognized_units="$recognized_units $name "
        obj="$out/${name}_gpu_$arch.o"
        if [ "${MAC_RENDERED_SESSION_INCREMENTAL:-0}" != 1 ] || \
            case "$recompile" in *" $name "*) true ;; *) false ;; esac; then
            "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
                -std=gnu89 -O2 $fp -Wall -Wextra -D__int64='long long' -D__fastcall= \
                -ffunction-sections -fdata-sections \
                -Iport/mac -Iport/include/xdk -Iport/linux/include -Iport/linux/src \
                -Iport/third_party/monocypher \
                -c "$unit" -o "$obj" > "$obj.txt" 2>&1
        elif [ ! -f "$obj" ]; then
            echo "incremental object is missing: $obj" >&2
            exit 1
        fi
        objects="$objects $obj"
        source_units="$source_units $unit"
    done

    # engine_files reads paths.data and paths.saves (the game's file flags)
    name=engine_files
    recognized_units="$recognized_units $name "
    obj="$out/${name}_gpu_$arch.o"
    if [ "${MAC_RENDERED_SESSION_INCREMENTAL:-0}" != 1 ] || \
        case "$recompile" in *" $name "*) true ;; *) false ;; esac; then
        "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
            -ffunction-sections -fdata-sections -I. -Isource -Isource/cseries \
            -Isource/math -Iport/mac -Iport/include/xdk -Iport/linux/game \
            -idirafter port/linux/include -c port/mac/engine_files.c \
            -o "$obj" > "$obj.txt" 2>&1
    elif [ ! -f "$obj" ]; then
        echo "incremental object is missing: $obj" >&2
        exit 1
    fi
    objects="$objects $obj"
    source_units="$source_units port/mac/engine_files.c"

    for unit in port/mac/mac_hud_hires.c "$out/mac_hud_assets.c" port/mac/mac_gpu_info.c port/mac/mac_input.c port/mac/mac_gamepad.c port/mac/mac_window.c port/mac/mac_audio_output.c port/mac/engine_messages.c port/mac/mac_gl_visibility.c port/mac/mac_gl_program_cache.c port/mac/mac_render_timing.c port/mac/mac_movie.c port/mac/mac_bink_movie.c port/linux/src/bink_decode.c port/linux/src/bink_tables.c port/mac/mac_loading.c; do
        name=$(basename "$unit" .c)
        recognized_units="$recognized_units $name "
        obj="$out/${name}_gpu_$arch.o"
        if [ "${MAC_RENDERED_SESSION_INCREMENTAL:-0}" != 1 ] || \
            case "$recompile" in *" $name "*) true ;; *) false ;; esac; then
            "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
                -std=gnu89 -O2 $fp -Wall -Wextra -Iport/mac -c "$unit" \
                -o "$obj" > "$obj.txt" 2>&1
        elif [ ! -f "$obj" ]; then
            echo "incremental object is missing: $obj" >&2
            exit 1
        fi
        objects="$objects $obj"
        source_units="$source_units $unit"
    done

    # Never silently retain a baseline object when a requested unit is not
    # part of this checker's explicit replacement graph.
    for requested in ${MAC_RENDERED_SESSION_RECOMPILE:-}; do
        case "$recognized_units" in
            *" $requested "*) ;;
            *) echo "unknown incremental unit: $requested" >&2; exit 1 ;;
        esac
    done

    # Real shared font atlas uploader, replacing the CPU-only text service.
    text_obj="$out/engine_text_gl_text_$arch.o"
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu99 -O2 $fp -I. -Iport/mac -Iport/linux/src -I"$audit" \
        -c port/mac/engine_text_gl.c -o "$text_obj" > "$text_obj.txt" 2>&1
    objects="$objects $text_obj"
    source_units="$source_units port/mac/engine_text_gl.c port/mac/engine_text.c"

    # Opt-in: recompile the game's listed prebuilt objects with extra flags
    # (for example -mtune=7450). Only objects outside this script's own units
    # whose basename matches exactly one source file are replaced; any failed
    # or ambiguous match keeps the listed object. Logged in $out/retune.log.
    if [ "$arch" = ppc ] && [ -n "${MAC_RENDERED_SESSION_RETUNE_FLAGS:-}" ]; then
        mkdir -p "$out/retuned"
        retune_log="$out/retune.log"
        echo "flags: ${MAC_RENDERED_SESSION_RETUNE_FLAGS}" > "$retune_log"
        retuned_objects=
        for obj in $objects; do
            case "$obj" in "$out"/*) retuned_objects="$retuned_objects $obj"; continue ;; esac
            stem=$(basename "$obj" "_$arch.o")
            # Objects from mac_spawn_link_checks.sh are named <unit>_spawn_link.
            stem=${stem%_spawn_link}
            case "$stem" in
                cseries|engine_stack|rasterizer_geometry|player_services|actor_type_grunt|errors|unicode|engine_services|pool_services)
                    echo "kept (special flags) $obj" >> "$retune_log"
                    retuned_objects="$retuned_objects $obj"; continue ;;
            esac
            case "$recognized_units" in *" $stem "*)
                echo "kept (own unit) $obj" >> "$retune_log"
                retuned_objects="$retuned_objects $obj"; continue ;;
            esac
            matches=$( { find source -name "$stem.c"; ls "port/linux/game/$stem.c" 2>/dev/null; } | sort -u )
            count=$(printf '%s\n' "$matches" | grep -c . || true)
            if [ "$count" != 1 ]; then
                echo "kept (matches=$count) $obj" >> "$retune_log"
                retuned_objects="$retuned_objects $obj"; continue
            fi
            new_obj="$out/retuned/${stem}_$arch.o"
            if compile_engine_unit "$arch" "$matches" "$new_obj" "$MAC_RENDERED_SESSION_RETUNE_FLAGS"; then
                echo "replaced $obj <- $matches" >> "$retune_log"
                retuned_objects="$retuned_objects $new_obj"
            else
                echo "kept (compile failed) $obj" >> "$retune_log"
                rm -f "$new_obj"
                retuned_objects="$retuned_objects $obj"
            fi
        done
        objects=$retuned_objects
    fi

    objects="$objects $out/mac_engine_session_probe_gpu_$arch.o"
    printf '%s\n' $objects > "$out/engine_objects_$arch.txt"
    {
        echo "base-manifest: $base/engine_objects_$arch.txt"
        echo "incremental: ${MAC_RENDERED_SESSION_INCREMENTAL:-0}"
        echo "recompiled-native-units: ${MAC_RENDERED_SESSION_RECOMPILE:-all}"
        echo "always-recompiled: mac_engine_session_probe engine_text_gl"
        echo "source-hashes: current checkout at link; reused objects retain prior build provenance"
        echo "generated-view-owners: recompile engine_device_state for state views, mac_dsound_native for audio view, and mac_gpu_textures for texture decode view when those generated includes change"
        shasum -a 256 "$base/engine_objects_$arch.txt"
        shasum -a 256 $objects
        shasum -a 256 "$audit/mac_text_body.h" "$audit/mac_text_gl_body.h"
        for source in $source_units tools/mac_engine_session_probe.c \
            tools/mac_rendered_session_checks.sh tools/mac_main_game_render_checks.sh \
            tools/mac_main_game_render_view.py \
            tools/mac_rasterizer_default_pixels_view.py tools/mac_device_state_view.py \
            tools/mac_xgpu_texture_decode_view.py tools/mac_text_boundary.py \
            tools/mac_dsound_view.py; do
            shasum -a 256 "$source"
        done
    } > "$out/source_provenance_$arch.txt"

    if "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -Wl,-dead_strip -Wl,-u,_mac_engine_main_game_render \
        $objects -lz -framework AudioUnit -framework AudioToolbox -framework CoreAudio \
        -framework CoreFoundation -framework Carbon -framework ApplicationServices \
        -framework AGL -framework OpenGL -framework IOKit -framework QuickTime -framework CoreVideo \
        -o "$out/mac_rendered_session_$arch" > "$out/link_$arch.txt" 2>&1; then
        echo "$arch: original session, rasterizer_initialize, main_game_render and present linked"
    else
        echo "$arch: rendered session link failed; see $out/link_$arch.txt" >&2
        exit 1
    fi
done

if [ "${MAC_RENDERED_SESSION_BUILD_ONLY:-0}" = 1 ]; then
    exit 0
fi

machine=$(uname -m)
case "$machine" in
    ppc|Power\ Macintosh) arch=ppc ;;
    i386|x86_64) arch=i386 ;;
    *) echo "unsupported native architecture for rendered session: $machine" >&2; exit 2 ;;
esac
test -x "$out/mac_rendered_session_$arch"
if [ -z "${HALO_MAC_DATA_ROOT:-}" ]; then
    map_dir=$(dirname "$1")
    if [ "$(basename "$map_dir" | tr '[:upper:]' '[:lower:]')" = maps ]; then
        HALO_MAC_DATA_ROOT=$(dirname "$map_dir")
        export HALO_MAC_DATA_ROOT
    fi
fi
created_save_root=
if [ -z "${HALO_MAC_SAVE_ROOT:-}" ]; then
    created_save_root=$(mktemp -d "${TMPDIR:-/tmp}/opence-rendered-save.XXXXXX")
    HALO_MAC_SAVE_ROOT=$created_save_root
    export HALO_MAC_SAVE_ROOT
fi
cleanup() { if [ -n "$created_save_root" ]; then rm -rf "$created_save_root"; fi; }
trap cleanup EXIT HUP INT TERM
export HALO_MAC_RENDER_CAPTURE=${HALO_MAC_RENDER_CAPTURE:-$out/original-game-frame-$arch.ppm}
"$out/mac_rendered_session_$arch" "$1" "$2" "$3" --gpu-render
