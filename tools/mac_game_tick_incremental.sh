#!/bin/sh
# Relink a native session manifest after compiling only changed provider units.
# Inputs are an existing mac_game_tick_checks.sh output directory and a
# whitespace-separated source list in MAC_GAME_TICK_RECOMPILE.
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
base=${MAC_GAME_TICK_BASE:?set MAC_GAME_TICK_BASE to a prior full-check output}
out=${MAC_GAME_TICK_OUT:?set MAC_GAME_TICK_OUT to a new output directory}
# An empty list is useful for relinking sources that this helper compiles
# directly at link time, without recompiling any manifest object.
units=${MAC_GAME_TICK_RECOMPILE-}
remove_names=${MAC_GAME_TICK_REMOVE:-}
replace_session_probe=no
replace_engine_config=no
case " $units " in *" tools/mac_engine_session_probe.c "*) replace_session_probe=yes ;; esac
case " $units " in *" port/mac/engine_config.c "*) replace_engine_config=yes ;; esac
for unit in $units; do
    case "$unit" in
        port/mac/mapped_file.c|port/mac/retail_cache.c|port/mac/retail_geometry.c|\
        port/mac/retail_texture.c|port/mac/retail_spawns.c|port/mac/retail_skeleton.c|\
        port/mac/retail_object_spawn.c|port/mac/native_tags.c|\
        port/mac/retail_collision_model.c|port/mac/retail_collision.c|\
        port/mac/retail_animation.c|port/mac/retail_animation_graph.c)
            echo "error: $unit is linked directly by this helper; do not add it to MAC_GAME_TICK_RECOMPILE" >&2
            exit 2
            ;;
    esac
done
mkdir -p "$out"
case " $units " in *" source/cseries/errors.c "*) python tools/mac_errors_view.py source/cseries/errors.c "$out/mac_errors.c" ;; esac
status=0
for arch in ${MAC_ARCHS:-ppc}; do
    fp=; [ "$arch" != ppc ] || fp=-mno-fused-madd
    provenance="$out/source_provenance_$arch.txt"
    : > "$provenance"
    replaced_names=
    for unit in $units; do
        name=$(basename "$unit" .c)
        [ "$unit" != source/cseries/errors.c ] || name=errors
        replaced_names="$replaced_names ${name}_$arch.o"
    done
    old_objects=
    while IFS= read -r file; do
        [ -n "$file" ] || continue
        object_name=${file##*/}
        replace=no
        for replacement in $replaced_names; do
            [ "$object_name" = "$replacement" ] && replace=yes
        done
        for explicit_remove in $remove_names; do
            [ "$object_name" = "$explicit_remove" ] && replace=yes
        done
        if [ "$replace" = no ] && [ "$replace_session_probe" = yes ]; then
            [ "$object_name" = "mac_game_tick_probe_$arch.o" ] && replace=yes
        fi
        if [ "$replace" = no ] && [ "$replace_engine_config" = yes ]; then
            case "$object_name" in engine_config_config_*.o) replace=yes ;; esac
        fi
        [ "$replace" = no ] || continue
        old_objects="$old_objects $file"
    done < "$base/engine_objects_$arch.txt"
    new_objects=
    for requested_unit in $units; do
        unit=$requested_unit
        [ "$requested_unit" != source/cseries/errors.c ] || unit="$out/mac_errors.c"
        name=$(basename "$unit" .c)
        extra_include=
        [ "$unit" != "$out/mac_errors.c" ] || extra_include="-include port/mac/engine_atomic_file.h"
        if ! "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $fp -fms-extensions -fshort-wchar -fno-strict-aliasing \
            -fwrapv -ffunction-sections -fdata-sections -I. -Isource -Isource/cseries \
            -Isource/math -Isource/physics -Isource/tag_files -Isource/memory -Isource/cache \
            -Isource/camera -Isource/bitmaps -Isource/main -Isource/editor -Isource/networking \
            -Isource/ai -Isource/interface -Isource/rasterizer -Isource/rasterizer/xbox \
            -Isource/text -Isource/cutscene -Isource/shaders -Isource/game -Isource/input \
            -Isource/objects -Isource/items -Isource/units -Isource/models -Isource/scenario \
            -Isource/structures -Isource/render -Isource/sound -Isource/effects -Isource/shell \
            -Isource/bink -I"source/saved games" -I"$out" -Ibuild/mac-audit -Iport/mac \
            -Iport/linux/game -Iport/linux/src -Iport/third_party/stb_vorbis \
            -idirafter port/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h \
            -include port/linux/include/halo_port_capacity.h \
            -include port/mac/engine_xdk_prefix.h -include build/mac-audit/mac_spawn_witness_names.h \
            -include port/include/halo_math.h -include port/linux/include/halo_linux_source_fixups.h \
            -include build/mac-audit/mac_engine_semantics.h $extra_include -c "$unit" \
            -o "$out/${name}_$arch.o" > "$out/${name}_$arch.txt" 2>&1; then
            echo "$arch compile failed: $unit" >&2
            cat "$out/${name}_$arch.txt" >&2
            status=1
            continue
        fi
        {
            echo "source $requested_unit"
            shasum -a 256 "$requested_unit"
            [ "$unit" = "$requested_unit" ] || {
                echo "generated-source $unit"
                shasum -a 256 "$unit"
            }
            # Hash sibling headers as a bounded, useful record of the source
            # unit's local ABI/schema inputs without rebuilding the manifest.
            unit_dir=$(dirname "$requested_unit")
            for header in "$unit_dir"/*.h; do
                [ -f "$header" ] || continue
                shasum -a 256 "$header"
            done
            echo "shared-compile-headers"
            shasum -a 256 port/linux/include/halo_port_limits.h \
                port/linux/include/halo_port_capacity.h port/mac/engine_xdk_prefix.h \
                port/include/halo_math.h build/mac-audit/mac_engine_semantics.h
        } >> "$provenance"
        new_objects="$new_objects $out/${name}_$arch.o"
    done
    objects="$old_objects$new_objects"
    printf '%s\n' $objects > "$out/engine_objects_$arch.txt"
    sh tools/mac_wide_import_check.sh "$out/engine_objects_$arch.txt"
    if ! "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -Iport/mac -Wl,-dead_strip $objects \
        -framework AudioUnit -framework AudioToolbox -framework CoreAudio \
        -framework CoreFoundation -framework Carbon -framework ApplicationServices \
        -framework AGL -framework OpenGL \
        port/mac/mapped_file.c port/mac/retail_cache.c port/mac/retail_geometry.c \
        port/mac/retail_texture.c port/mac/retail_spawns.c port/mac/retail_skeleton.c \
        port/mac/retail_object_spawn.c port/mac/native_tags.c port/mac/retail_collision_model.c \
        port/mac/retail_collision.c port/mac/retail_animation.c \
        port/mac/retail_animation_graph.c -o "$out/game_tick_link_$arch" \
        > "$out/link_$arch.txt" 2>&1; then
        echo "$arch: incremental session link incomplete; see $out/link_$arch.txt"
        status=1
    else
        echo "$arch: incremental session link succeeded"
    fi
done
exit "$status"
