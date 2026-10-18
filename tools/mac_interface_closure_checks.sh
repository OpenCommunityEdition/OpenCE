#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit/interface-closure}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    objects=
    for unit in port/mac/native_interface_assets.c port/mac/native_interface_dependencies.c \
        port/mac/native_hud_messages.c port/mac/native_font_tags.c port/mac/native_object_tags.c \
        port/mac/native_actor_assets.c port/mac/native_bitmap_tags.c port/mac/engine_services.c \
        port/mac/engine_texture_cache.c tools/mac_interface_closure_probe.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $fp -ffunction-sections -fdata-sections -fms-extensions \
            -fshort-wchar -fno-strict-aliasing -fwrapv \
            -I. -Isource -Isource/cseries -Isource/math -Isource/tag_files \
            -Isource/interface -Isource/bitmaps -Isource/structures -Isource/sound -Iport/mac \
            -Iport/linux/include -Iport/include/xdk -Iport/include \
            -include port/linux/include/halo_port_limits.h \
            -include port/linux/include/halo_port_capacity.h \
            -include port/mac/engine_xdk_prefix.h -include port/include/halo_math.h \
            -include port/linux/include/halo_linux_source_fixups.h \
            -include "$mac_dir/mac_spawn_witness_names.h" \
            -include "$mac_dir/mac_engine_semantics.h" \
            -c "$unit" -o "$mac_dir/${name}_${arch}.o"
        objects="$objects $mac_dir/${name}_${arch}.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -Iport/mac -Wl,-dead_strip $objects port/mac/retail_cache.c port/mac/native_tags.c \
        port/mac/mapped_file.c port/mac/retail_texture.c port/mac/engine_format.c \
        -o "$mac_dir/interface_closure_${arch}"
done
"$mac_dir/interface_closure_i386" "$@"
