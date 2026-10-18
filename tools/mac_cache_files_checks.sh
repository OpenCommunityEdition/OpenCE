#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    for unit in port/mac/engine_cache_files.c tools/mac_cache_files_probe.c; do
        name=$(basename "$unit" .c)
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 \
            -fshort-wchar -I. -Isource -Isource/cseries -Isource/math -Isource/tag_files -Iport/mac \
            -include port/mac/engine_prefix.h -include "$mac_dir/mac_engine_semantics.h" \
            -c "$unit" -o "$mac_dir/${name}_${arch}.o"
    done
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 \
        -Iport/mac -c port/mac/mapped_file.c -o "$mac_dir/mapped_file_cache_files_${arch}.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 \
        -Iport/mac -c port/mac/retail_cache.c -o "$mac_dir/retail_cache_files_${arch}.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 \
        -Iport/mac -Wl,-dead_strip "$mac_dir/engine_cache_files_${arch}.o" "$mac_dir/mac_cache_files_probe_${arch}.o" \
        "$mac_dir/mapped_file_cache_files_${arch}.o" "$mac_dir/retail_cache_files_${arch}.o" -o "$mac_dir/cache_files_$arch"
done
"$mac_dir/cache_files_i386" "$@"
