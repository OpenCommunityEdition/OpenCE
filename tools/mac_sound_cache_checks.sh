#!/bin/sh
# Exercise the original sound cache with owned retail sounds and real mapped
# files. A combined native engine object manifest supplies unchanged providers;
# changed cache/loader/probe units are always rebuilt together here.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
probe_src=${MAC_NATIVE_PROBE:-tools/mac_sound_tags_probe.c}
baseline=${MAC_SOUND_BASELINE:-build/mac-game-tick-cycle12}
old_main=mac_game_tick_probe
out=${MAC_SOUND_CACHE_OUT:-build/mac-sound-cache}
base=build/mac-audit
mkdir -p "$out"
python tools/mac_lruv_view.py "$out/mac_lruv_cache.c"
python tools/mac_game_state_storage_view.py "$out/mac_game_state_storage.c"
for arch in ${MAC_ARCHS:-ppc i386}; do
    fp=;if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    apply_flags=-DMAC_SOUND_CACHE_LIFECYCLE;service_alias=;view_frameworks="-framework Carbon -framework AGL -framework OpenGL -framework AudioUnit -framework AudioToolbox -framework CoreAudio"
    objects=
    for file in $(cat "$baseline/engine_objects_$arch.txt"); do
        case "$file" in */engine_arena_$arch.o|*/${old_main}_$arch.o|*/native_object_tags_$arch.o|*/engine_cache_files_$arch.o|*/cache_files_$arch.o|*/sound_dsound_xbox_$arch.o|*/physical_memory_map_$arch.o|*/xbox_sound_cache_$arch.o|*/lruv_cache_*.o|*/game_state_xbox_*.o|*/mac_game_state_storage_*.o) continue ;; esac
        objects="$objects $file"
    done
    for unit in port/mac/engine_arena.c "$probe_src" port/mac/engine_tags.c port/mac/native_object_tags.c port/mac/engine_cache_files.c source/cache/physical_memory_map.c source/cache/xbox_sound_cache.c "$out/mac_lruv_cache.c" "$out/mac_game_state_storage.c"; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp $apply_flags $service_alias \
            -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files -Isource/memory \
            -Isource/cache -Isource/camera -Isource/bitmaps -Isource/main -Isource/editor -Isource/networking -Isource/ai -Isource/interface -Isource/rasterizer -Isource/rasterizer/xbox -Isource/text -Isource/cutscene -Isource/shaders -Isource/game -Isource/input -Isource/objects -Isource/items -Isource/units -Isource/models \
            -Isource/scenario -Isource/structures -Isource/render -Isource/sound -Isource/effects -Isource/shell -Isource/bink \
            -I"source/saved games" -I"$out" -I"$base" -Iport/mac -Iport/linux/game -idirafter port/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h -include port/mac/engine_xdk_prefix.h \
            -include "$base/mac_spawn_witness_names.h" -include port/include/halo_math.h \
            -include port/linux/include/halo_linux_source_fixups.h -include "$base/mac_engine_semantics.h" \
            -c "$unit" -o "$out/${name}_$arch.o"
        objects="$objects $out/${name}_$arch.o"
    done
    printf "%s\n" $objects > "$out/objects_$arch.txt"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -Iport/mac -Wl,-dead_strip $objects port/mac/mapped_file.c port/mac/retail_cache.c port/mac/native_tags.c $view_frameworks -o "$out/probe_$arch" > "$out/link_$arch.txt" 2>&1
done
if ! "$out/probe_i386" "$@" > "$out/run_i386.txt" 2>&1; then
    cat "$out/run_i386.txt";exit 1
fi
cat "$out/run_i386.txt"
if LC_ALL=C grep -E 'malloc:|FAIL |engine assertion|native engine stack:|invalid native tag|XPhysicalFree refused|XPhysicalProtect refused' "$out/run_i386.txt" >/dev/null; then
    echo "Native sound cache emitted runtime diagnostics" >&2;exit 1
fi
