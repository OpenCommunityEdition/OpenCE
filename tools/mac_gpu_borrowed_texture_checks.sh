#!/bin/sh
set -eu

cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_GPU_BORROWED_OUT:-build/mac-audit/gpu-borrowed-texture}
objects_dir=${MAC_GPU_BORROWED_OBJECT_DIR:-build/mac-audit/rendered-session/volume-render}
script_dir=$(dirname "$0")
mkdir -p "$out"
overlay="$out/overlay"
mkdir -p "$overlay"

for arch in i386 ppc; do
    manifest="$objects_dir/engine_objects_$arch.txt"
    test -f "$manifest"
    fp=
    if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -DMAC_SESSION_GPU_RENDER -fms-extensions \
        -fshort-wchar -fno-strict-aliasing -fwrapv \
        -I"$overlay" -I"$objects_dir" -I"$objects_dir/render-graph" \
        -Ibuild/mac-audit/rendered-session/volume-render \
        -Ibuild/mac-audit/rendered-session/volume-render/render-graph \
        -Ibuild/mac-audit -I. -Isource -Isource/cseries -Isource/math \
        -Isource/physics -Isource/tag_files -Isource/memory -Isource/cache \
        -Isource/camera -Isource/bitmaps -Isource/main -Isource/editor \
        -Isource/networking -Isource/ai -Isource/interface -Isource/rasterizer \
        -Isource/rasterizer/xbox -Isource/text -Isource/cutscene -Isource/shaders \
        -Isource/game -Isource/input -Isource/objects -Isource/items -Isource/units \
        -Isource/models -Isource/scenario -Isource/structures -Isource/render \
        -Isource/sound -Isource/effects -Isource/shell -Isource/bink \
        -I"source/saved games" -Iport/mac -Iport/linux/game -Iport/linux/src \
        -Iport/third_party/stb_vorbis -idirafter port/linux/include -Iport/include/xdk \
        -include port/linux/include/halo_port_limits.h \
        -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_xdk_prefix.h \
        -include build/mac-audit/mac_engine_semantics.h \
        -include port/include/halo_math.h \
        -c "$script_dir/mac_gpu_borrowed_texture_probe.c" -o "$out/probe_$arch.o"
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
        -I"$overlay" -I"$objects_dir" -I"$objects_dir/render-graph" \
        -Ibuild/mac-audit -I. -Isource -Isource/cseries -Isource/math \
        -Isource/physics -Isource/tag_files -Isource/scenario -Isource/structures \
        -Iport/linux/src -Iport/linux/game -I"source/saved games" -Isource/units \
        -Isource/render -Isource/sound -Isource/main -Isource/interface \
        -Isource/rasterizer -Isource/cutscene -Isource/ai -Isource/cache \
        -Isource/camera -Isource/bitmaps -Isource/models -Isource/items \
        -Isource/effects -Isource/memory -Isource/game -Isource/input -Isource/objects \
        -Iport/mac -Iport/linux/game -idirafter port/linux/include -Iport/include/xdk \
        -include port/linux/include/halo_port_limits.h \
        -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_xdk_prefix.h \
        -include build/mac-audit/mac_engine_semantics.h \
        -include port/include/halo_math.h -c "$overlay/mac_gpu_textures.c" \
        -o "$out/mac_gpu_textures_gpu_$arch.o"
    objects=
    while IFS= read -r object; do
        case "$object" in
            *mac_engine_session_probe_gpu_"$arch".o) continue ;;
            *mac_gpu_textures_gpu_"$arch".o) continue ;;
        esac
        test -f "$object"
        objects="$objects $object"
    done < "$manifest"
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -Wl,-dead_strip -framework Carbon -framework AGL \
        -framework OpenGL $objects "$out/mac_gpu_textures_gpu_$arch.o" \
        "$out/probe_$arch.o" \
        -o "$out/mac_gpu_borrowed_texture_$arch"
    nm -m -u "$out/mac_gpu_borrowed_texture_$arch" | grep 'weak' && {
        echo "Unresolved weak import in $arch borrowed texture probe" >&2
        exit 1
    }
done

for arch in i386 ppc; do
    binary="$out/mac_gpu_borrowed_texture_$arch"
    (unset HALO_MAC_RETAIL_TEXTURE_CACHE_16M; "$binary")
    HALO_MAC_RETAIL_TEXTURE_CACHE_16M=1 "$binary"
    HALO_MAC_RETAIL_TEXTURE_CACHE_16M=enabled "$binary" --invalid-cache-setting
    HALO_MAC_RETAIL_TEXTURE_CACHE_16M= "$binary" --invalid-cache-setting
done
