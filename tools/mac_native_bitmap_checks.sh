#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
python tools/mac_resource_release_body.py build/mac-audit/mac_resource_release_body.inc
python tools/mac_resource_boundary.py
for arch in ppc i386; do
    mac_fp=
    if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
    objects=
    for unit in port/mac/shared_d3d_resources.c port/mac/shared_texture_layout.c port/mac/native_resource_memory.c port/mac/engine_bitmaps.c source/bitmaps/s3tc/s3tc.c source/rasterizer/rasterizer_swizzle.c source/math/integer_math.c port/mac/player_services.c port/mac/native_bitmap_tags.c port/mac/native_tags.c port/mac/engine_texture_cache.c port/mac/engine_services.c tools/mac_native_bitmap_probe.c; do
        name=$(basename "$unit" .c)
        mac_prefix=port/mac/engine_xdk_prefix.h
        mac_alias=
        mac_testing=
        if [ "$unit" = port/mac/native_tags.c ] || [ "$unit" = tools/mac_native_bitmap_probe.c ]; then
            mac_testing=-DHALO_MAC_NATIVE_TAGS_TESTING
        fi
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $mac_fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv $mac_alias $mac_testing \
            -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files -Isource/scenario -Isource/structures \
            -I"source/saved games" -Isource/units -Isource/render -Isource/sound -Isource/main -Isource/interface -Isource/rasterizer -Isource/cutscene -Isource/ai -Isource/cache -Isource/camera -Isource/bitmaps -Isource/models -Isource/items -Isource/effects -Isource/memory -Isource/game -Isource/input -Isource/objects -Iport/mac -Iport/linux/game -idirafter port/linux/include -Iport/include/xdk \
            -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
            -include "$mac_prefix" -include port/include/halo_math.h -include "$mac_dir/mac_engine_semantics.h" \
            -c "$unit" -o "$mac_dir/${name}_native_bitmap_${arch}.o"
        objects="$objects $mac_dir/${name}_native_bitmap_${arch}.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -fno-strict-aliasing -Iport/mac -Wl,-dead_strip \
        $objects port/mac/mapped_file.c port/mac/retail_cache.c port/mac/retail_texture.c \
        -o "$mac_dir/native_bitmap_$arch"
    nm -u "$mac_dir/native_bitmap_$arch" > "$mac_dir/native_bitmap_imports_$arch.txt"
    if nm -m -u "$mac_dir/native_bitmap_$arch" | grep 'weak'; then
        echo "Unresolved weak engine import" >&2; exit 1
    fi
done
"$mac_dir/native_bitmap_i386" "$@"
