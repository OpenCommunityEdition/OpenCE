#!/bin/sh
# The raw data blobs of real Xbox maps (xbox_blob.c) in both byte orders; PowerPC runs under
# Rosetta on an Intel Mac. python3 lists the blobs (the Intel build Mac has python 2.6: set MAC_BLOB_LISTS to ready lists).
# usage: sh tools/mac_xbox_blob_checks.sh map...
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_XBOX_CHECK_OUT:-build/mac-xbox-blob}
mkdir -p "$out"
engine_flags() {
    echo -DHALO_MAC -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv -I. -Isource -Isource/cseries -Isource/math \
        -Isource/physics -Isource/tag_files -Isource/memory -Isource/cache -Isource/bitmaps -Isource/rasterizer \
        -Isource/models -Isource/structures -Isource/render -Isource/scenario -Isource/objects -Isource/game \
        -Iport/mac -Iport/linux/game -Iport/linux/src -idirafter port/linux/include -Iport/include/xdk \
        -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_xdk_prefix.h -include port/include/halo_math.h -include port/linux/include/halo_linux_source_fixups.h
}
for arch in ${MAC_ARCHS:-i386 ppc}; do
    fp=; [ "$arch" = ppc ] && fp=-mno-fused-madd
    base="-arch $arch -isysroot $sdk -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp"
    # shellcheck disable=SC2046
    "$cc" $base $(engine_flags) -c port/mac/xbox_geometry.c -o "$out/geometry_$arch.o"
    for f in port/mac/xbox_blob tools/mac_xbox_blob_probe port/mac/xbox_map port/mac/mapped_file port/mac/cache_endian; do
        "$cc" $base -Iport/mac -c $f.c -o "$out/$(basename $f)_$arch.o"
    done
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 $fp "$out/geometry_$arch.o" "$out/xbox_blob_$arch.o" "$out/mac_xbox_blob_probe_$arch.o" \
        "$out/xbox_map_$arch.o" "$out/mapped_file_$arch.o" "$out/cache_endian_$arch.o" -lz -Wl,-dead_strip -o "$out/xbox_blob_probe_$arch"
done
for map in "$@"; do
    name=$(basename "$map" .map)
    list=${MAC_BLOB_LISTS:-$out}/$name.blobs
    [ -f "$list" ] || python3 tools/mac_xbox_blob_list.py "$map" > "$list"
    for arch in ${MAC_ARCHS:-i386 ppc}; do
        if [ "$arch" = i386 ]; then HALO_MAC_XBOX_FORCE_SWAP=1 "$out/xbox_blob_probe_$arch" "$map" "$list" > "$out/$name.blob.$arch.txt"
        else "$out/xbox_blob_probe_$arch" "$map" "$list" > "$out/$name.blob.$arch.txt"; fi
    done
    cmp "$out/$name.blob.i386.txt" "$out/$name.blob.ppc.txt" > /dev/null && echo "$name: $(cat "$out/$name.blob.i386.txt"); same in both byte orders"
done
