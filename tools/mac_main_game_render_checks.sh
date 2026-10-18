#!/bin/sh
# Link the original main_game_render/render_frame graph into the real-session
# object manifest. This is a link gate only; it does not execute the renderer.
set -eu

cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
manifest=${MAC_RENDER_MANIFEST:-build/mac-game-tick-session-final36}
audit=${MAC_RENDER_AUDIT_BASE:-build/mac-audit}
out=${MAC_RENDER_OUT:-build/mac-render-frame-audit}
mkdir -p "$out"

test -f "$manifest/engine_objects_i386.txt"
test -f "$manifest/engine_objects_ppc.txt"
test -f "$audit/mac_engine_semantics.h"
test -f "$audit/mac_spawn_witness_names.h"
python tools/mac_main_game_render_view.py "$out/mac_main_game_render.c"

for arch in ${MAC_ARCHS:-ppc i386}; do
    fp=
    if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    objects=
    # This source view owns main.c's complete function set and adds the public
    # render wrapper, so omit the older generated main.c object. Recompile the
    # debug globals because this view carries the latest typed BSS inventory.
    for obj in $(cat "$manifest/engine_objects_$arch.txt"); do
        case "$obj" in
            */main_spawn_link_*.o|*/engine_debug_globals_*.o) continue ;;
        esac
        objects="$objects $obj"
    done

    compile() {
        source=$1
        output=$2
        extra=${3:-}
        # The source set comes from the engine's original render_frame path.
        # Function/data sections let the final link retain only reachable work.
        # shellcheck disable=SC2086
        "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $fp $extra -fms-extensions -fshort-wchar \
            -fno-strict-aliasing -fwrapv -ffunction-sections -fdata-sections \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics \
            -Isource/tag_files -Isource/memory -Isource/cache -Isource/camera \
            -Isource/bitmaps -Isource/main -Isource/editor -Isource/networking \
            -Isource/ai -Isource/interface -Isource/rasterizer \
            -Isource/rasterizer/xbox -Isource/text -Isource/cutscene \
            -Isource/shaders -Isource/game -Isource/input -Isource/objects \
            -Isource/items -Isource/units -Isource/models -Isource/scenario \
            -Isource/structures -Isource/render -Isource/sound -Isource/effects \
            -Isource/shell -Isource/bink -I"source/saved games" -I"$out" \
            -I"$audit" -Iport/mac -Iport/linux/game -Iport/linux/src \
            -Iport/third_party/stb_vorbis -idirafter port/linux/include \
            -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h \
            -include port/linux/include/halo_port_capacity.h \
            -include port/mac/engine_xdk_prefix.h \
            -include "$audit/mac_spawn_witness_names.h" \
            -include port/include/halo_math.h \
            -include port/linux/include/halo_linux_source_fixups.h \
            -include "$audit/mac_engine_semantics.h" \
            -c "$source" -o "$output" > "$output.txt" 2>&1
    }

    compile "$out/mac_main_game_render.c" "$out/mac_main_game_render_$arch.o"
    compile port/mac/engine_render_metrics.c "$out/engine_render_metrics_$arch.o"
    compile port/mac/engine_render_services.c "$out/engine_render_services_$arch.o"
    compile port/mac/engine_debug_globals.c "$out/engine_debug_globals_$arch.o" \
        -Dfast_ftol=mac_render_debug_globals_unused_fast_ftol

    for unit in \
        source/render/render_sky.c \
        source/render/render_particles.c \
        source/render/render_contrails.c \
        source/structures/structure_render.c \
        source/rasterizer/xbox/rasterizer_xbox_environment.c \
        source/rasterizer/xbox/rasterizer_xbox_detail_objects.c \
        source/rasterizer/xbox/rasterizer_xbox_shadows.c \
        source/rasterizer/xbox/rasterizer_xbox_debug.c \
        source/rasterizer/xbox/rasterizer_xbox_motion_sensor.c \
        source/rasterizer/xbox/rasterizer_xbox_screen_effect.c \
        source/rasterizer/xbox/rasterizer_xbox_lights.c \
        source/structures/leaf_map.c \
        port/linux/game/shield_glow.c; do
        name=$(basename "$unit" .c)
        compile "$unit" "$out/${name}_$arch.o"
    done

    objects="$objects $out/mac_main_game_render_$arch.o"
    objects="$objects $out/engine_render_metrics_$arch.o"
    objects="$objects $out/engine_render_services_$arch.o"
    objects="$objects $out/engine_debug_globals_$arch.o"
    for unit in render_sky render_particles render_contrails structure_render \
        rasterizer_xbox_environment rasterizer_xbox_detail_objects \
        rasterizer_xbox_shadows rasterizer_xbox_debug \
        rasterizer_xbox_motion_sensor rasterizer_xbox_screen_effect \
        rasterizer_xbox_lights leaf_map shield_glow; do
        objects="$objects $out/${unit}_$arch.o"
    done

    retail_sources='port/mac/mapped_file.c port/mac/retail_cache.c port/mac/retail_geometry.c port/mac/retail_texture.c port/mac/retail_spawns.c port/mac/retail_skeleton.c port/mac/retail_object_spawn.c port/mac/native_tags.c port/mac/retail_collision_model.c port/mac/retail_collision.c port/mac/retail_animation.c port/mac/retail_animation_graph.c'
    source_units="port/mac/engine_render_metrics.c port/mac/engine_render_services.c port/mac/engine_debug_globals.c source/render/render_sky.c source/render/render_particles.c source/render/render_contrails.c source/structures/structure_render.c source/rasterizer/xbox/rasterizer_xbox_environment.c source/rasterizer/xbox/rasterizer_xbox_detail_objects.c source/rasterizer/xbox/rasterizer_xbox_shadows.c source/rasterizer/xbox/rasterizer_xbox_debug.c source/rasterizer/xbox/rasterizer_xbox_motion_sensor.c source/rasterizer/xbox/rasterizer_xbox_screen_effect.c source/rasterizer/xbox/rasterizer_xbox_lights.c source/structures/leaf_map.c port/linux/game/shield_glow.c $retail_sources"
    printf '%s\n' $objects > "$out/engine_objects_$arch.txt"
    {
        echo "base-manifest: $manifest/engine_objects_$arch.txt"
        shasum -a 256 "$manifest/engine_objects_$arch.txt"
        for source in $source_units; do
            shasum -a 256 "$source"
        done
        shasum -a 256 "$out/mac_main_game_render.c"
        shasum -a 256 tools/mac_main_game_render_view.py
        shasum -a 256 tools/mac_debug_globals.py port/mac/native_debug_globals.inc
        shasum -a 256 "$audit/mac_engine_semantics.h" "$audit/mac_spawn_witness_names.h"
    } > "$out/source_provenance_$arch.txt"

    # The forced symbol keeps the original wrapper and its real caller chain
    # alive under dead stripping; the session probe remains the sole main().
    if "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -Iport/mac -Wl,-dead_strip \
        -Wl,-u,_mac_engine_main_game_render $objects \
        -framework AudioUnit -framework AudioToolbox -framework CoreAudio \
        -framework CoreFoundation -framework Carbon \
        -framework ApplicationServices -framework AGL -framework OpenGL \
        $retail_sources \
        -o "$out/session_plus_render_$arch" \
        > "$out/link_$arch.txt" 2>&1; then
        echo "$arch: original main_game_render/render_frame graph linked; not executed"
    else
        echo "$arch: render graph link failed; see $out/link_$arch.txt" >&2
        exit 1
    fi
done
