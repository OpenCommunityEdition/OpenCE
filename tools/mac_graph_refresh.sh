#!/bin/sh
# Keep the session's prebuilt render-graph objects current with the sources.
#
# The session links the objects listed in $out/render-graph/engine_objects_<arch>.txt: units compiled long
# ago by other gates (build/mac-game-tick, build/mac-audit, build/mac-graphics-audit), which nothing
# rebuilds when a source changes. This compiles every such unit that lives under source/ or
# port/linux/game from the current tree, with the flags of the gate that made the object, and compares
# it with the prebuilt one (the disassembled text, relocations, strings and data). A unit whose code
# differs is stale: the fresh object takes its place in $out/render-graph/engine_objects_refreshed_<arch>.txt,
# which the session script links; one that is the same keeps its prebuilt object (so the result is
# unchanged by this step wherever nothing changed). Reports: $out/graph-fresh/report_<arch>.txt
#   stale <object> <source>      the fresh object is linked
#   same <object>                nothing to do
#   kept <object> <why>          no source found, more than one candidate, or it does not compile here
# A unit is compiled again only when its preprocessed text or flags change (graph-fresh/<name>.stamp).
# usage: mac_graph_refresh.sh OUT AUDIT       (MAC_ARCHS, MAC_CC, MAC_SDK as in the other gates)
set -eu
out=$1
audit=$2
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
fresh="$out/graph-fresh"
mkdir -p "$fresh"

# the one source of an object's name, or nothing (several, or none)
find_source() {
    n=$1
    found=
    count=0
    # port: engine_arena (port/mac) includes source/saved games/game_state.c, so a change to that file
    # (the gamestate.txt log) reaches the build only through this unit
    if [ "$n" = engine_arena ] && [ -f port/mac/engine_arena.c ]; then
        printf '%s\n' port/mac/engine_arena.c
        return 0
    fi
    # port: mac_constructor_storage (tools/) includes source/objects/objects.c, the objects' tick (node matrices,
    # lights, updates); without this a change to objects.c never reached a build
    if [ "$n" = mac_constructor_storage ] && [ -f tools/mac_constructor_storage.c ]; then
        printf '%s\n' tools/mac_constructor_storage.c
        return 0
    fi
    for f in source/*/"$n".c source/*/*/"$n".c "source/saved games/$n.c" port/linux/game/"$n".c; do
        [ -f "$f" ] || continue
        found=$f
        count=$((count+1))
    done
    [ "$count" -eq 1 ] && printf '%s\n' "$found"
    return 0
}

fingerprint() {
    # text, relocations, strings and data of an object, as one checksum
    { otool -tv "$1" | sed 1,2d; otool -rv "$1" 2>/dev/null | sed 1d; otool -s __TEXT __cstring "$1" 2>/dev/null | sed 1d;
      otool -s __DATA __data "$1" 2>/dev/null | sed 1d; otool -s __DATA __const "$1" 2>/dev/null | sed 1d; } | md5 -q
}

status=0
for arch in ${MAC_ARCHS:-ppc i386}; do
    list="$out/render-graph/engine_objects_$arch.txt"
    refreshed="$out/render-graph/engine_objects_refreshed_$arch.txt"
    report="$fresh/report_$arch.txt"
    fp=
    [ "$arch" != ppc ] || fp=-mno-fused-madd
    : > "$report"
    : > "$refreshed"
    for obj in $(cat "$list"); do
        base=$(basename "$obj" "_$arch.o")
        name=${base%_spawn_link}
        spawn=no
        [ "$name" = "$base" ] || spawn=yes
        src=
        case "$obj" in
            */mac-game-tick/*|*/mac-audit/*|*/mac-graphics-audit/*) src=$(find_source "$name") ;;
        esac
        if [ -z "$src" ]; then
            echo "kept $obj (no single source under source/ or port/linux/game)" >> "$report"
            echo "$obj" >> "$refreshed"
            continue
        fi
        # the flags of the gate that built it
        alias=
        pre="-include port/mac/engine_xdk_prefix.h"
        post=
        case "$src" in
            source/interface/hud_draw.c) alias="-D__INTPTR_TYPE__=long" ;;
            source/interface/ui_widget.c) post="-include port/mac/engine_wide_redirect.h" ;;
            source/hs/hs.c) post="-include port/mac/mac_ctype_compat.h" ;;
            source/game/game_engine_king.c) post="-include port/mac/mac_game_engine_king_compat.h" ;;
            source/cseries/errors.c) post="-include port/mac/engine_atomic_file.h" ;;
            source/rasterizer/rasterizer_text.c) post="-include port/mac/engine_rasterizer_text_prefix.h" ;;
        esac
        if [ "$spawn" = yes ]; then
            case "$src" in
                source/cseries/cseries.c) alias="-DBUILDING_CSERIES" ;;
                source/rasterizer/rasterizer_geometry.c) pre="-include port/mac/engine_prefix.h -include port/mac/native_geometry_math.h -include port/mac/engine_xdk_prefix.h" ;;
                source/ai/actor_type_grunt.c) post="-include port/mac/engine_declspec_align.h" ;;
                source/text/unicode.c) post="-include port/mac/engine_wide_redirect.h" ;;
            esac
        fi
        flags="-arch $arch -isysroot $sdk -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp $alias -fms-extensions -fshort-wchar \
-fno-strict-aliasing -fwrapv -ffunction-sections -fdata-sections -I$audit -I. -Isource -Isource/cseries -Isource/math \
-Isource/physics -Isource/tag_files -Isource/memory -Isource/cache -Isource/camera -Isource/bitmaps -Isource/main \
-Isource/editor -Isource/networking -Isource/ai -Isource/interface -Isource/rasterizer -Isource/rasterizer/xbox -Isource/text \
-Isource/cutscene -Isource/shaders -Isource/game -Isource/input -Isource/objects -Isource/items -Isource/units -Isource/models \
-Isource/scenario -Isource/structures -Isource/render -Isource/sound -Isource/effects -Isource/shell -Isource/bink \
-I'source/saved games' -Iport/mac -Iport/linux/game -Iport/linux/src -Iport/third_party/stb_vorbis -idirafter port/linux/include \
-Iport/include/xdk -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h $pre \
-include $audit/mac_spawn_witness_names.h -include port/include/halo_math.h -include port/linux/include/halo_linux_source_fixups.h \
-include $audit/mac_engine_semantics.h $post"
        eval "set -- $flags"
        new="$fresh/${base}_$arch.o"
        stamp="$fresh/${base}_$arch.stamp"
        key=$({ echo "$flags"; "$cc" "$@" -E "$src" 2>/dev/null; } | md5 -q)
        if [ ! -f "$new" ] || [ ! -f "$stamp" ] || [ "$(cat "$stamp")" != "$key" ]; then
            rm -f "$new" "$fresh/${base}_$arch.verdict"
            if "$cc" "$@" -c "$src" -o "$new" > "$fresh/${base}_$arch.txt" 2>&1; then
                if [ "$(fingerprint "$new")" = "$(fingerprint "$obj")" ]; then
                    echo same > "$fresh/${base}_$arch.verdict"
                else
                    echo stale > "$fresh/${base}_$arch.verdict"
                fi
            else
                echo "compile-failed" > "$fresh/${base}_$arch.verdict"
                rm -f "$new"
            fi
            echo "$key" > "$stamp"
        fi
        case "$(cat "$fresh/${base}_$arch.verdict")" in
            stale)
                echo "stale $obj $src" >> "$report"
                echo "$new" >> "$refreshed" ;;
            same)
                echo "same $obj" >> "$report"
                echo "$obj" >> "$refreshed" ;;
            *)
                echo "kept $obj ($src does not compile with the gate's flags here: see $fresh/${base}_$arch.txt)" >> "$report"
                echo "$obj" >> "$refreshed" ;;
        esac
    done
    echo "$arch: graph units $(grep -c '^stale' "$report") stale, $(grep -c '^same' "$report") current, $(grep -c '^kept' "$report") kept as built ($report)"
done
exit $status
