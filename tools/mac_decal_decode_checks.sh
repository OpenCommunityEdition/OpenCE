#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    fp=;if [ "$arch" = ppc ]; then fp=-mno-fused-madd;fi
    objects=
    for unit in port/mac/native_object_tags.c port/mac/engine_tags.c tools/mac_decal_decode_probe.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
            -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
            -I. -Isource -Isource/cseries -Isource/math -Isource/tag_files -Iport/mac \
            -include port/mac/engine_prefix.h -include "$mac_dir/mac_engine_semantics.h" \
            -c "$unit" -o "$mac_dir/${name}_decal_$arch.o"
        objects="$objects $mac_dir/${name}_decal_$arch.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
        -Iport/mac -Wl,-dead_strip $objects port/mac/retail_cache.c port/mac/native_tags.c \
        -o "$mac_dir/decal_decode_$arch"
done
"$mac_dir/decal_decode_i386" "$@"
