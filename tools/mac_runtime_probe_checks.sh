#!/bin/sh
# Replace one diagnostic/provider unit against a freshly built runtime manifest.
# Rebuild mac_object_runtime_checks.sh first after any engine/provider changes.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
unit=$1
baseline=$2
old_main=$3
out=$4
shift 4
base=build/mac-audit
mkdir -p "$out"
for arch in ${MAC_ARCHS:-ppc i386}; do
    fp=;if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    apply_flags=;service_alias=;view_frameworks=
    if [ "${MAC_RUNTIME_VIEW:-0}" = 1 ];then
        apply_flags="$apply_flags -DMAC_RUNTIME_VIEW"
        view_frameworks="-framework Carbon -framework AGL -framework OpenGL"
    fi
    if [ "${MAC_PLAYER_QUEUE_APPLY:-0}" = 1 ];then apply_flags="$apply_flags -DMAC_PLAYER_QUEUE_APPLY";fi
    name=$(basename "$unit" .c)
    objects=
    for file in $(cat "$baseline/objects_$arch.txt"); do
        case "$file" in */${old_main}_$arch.o) continue ;; esac
        objects="$objects $file"
    done
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
    printf "%s\n" $objects > "$out/objects_$arch.txt"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -Iport/mac -Wl,-dead_strip $objects \
        port/mac/mapped_file.c port/mac/retail_cache.c port/mac/retail_geometry.c port/mac/native_scenario_objects.c port/mac/native_scenario_blocks.c port/mac/retail_texture.c port/mac/retail_spawns.c port/mac/retail_skeleton.c port/mac/retail_object_spawn.c port/mac/native_tags.c port/mac/retail_collision_model.c port/mac/retail_collision.c port/mac/retail_animation.c port/mac/retail_animation_graph.c \
        $view_frameworks -o "$out/probe_$arch" > "$out/link_$arch.txt" 2>&1
done
"$out/probe_i386" "$@"
