#!/bin/sh
# The tag layout tables (port/mac/xbox_tag_tables.c) walked over real Xbox maps in both byte orders
# (PowerPC under Rosetta on an Intel Mac): bounds, overlaps, floats, coverage (tools/mac_xbox_tag_check.c).
# usage: sh tools/mac_xbox_tag_checks.sh map...
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_XBOX_CHECK_OUT:-build/mac-xbox-tags}
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
    for f in port/mac/xbox_tag_tables port/mac/xbox_tag_walk port/mac/xbox_blob port/mac/xbox_map port/mac/mapped_file port/mac/cache_endian tools/mac_xbox_tag_check; do
        "$cc" $base -Wall -Iport/mac -c $f.c -o "$out/$(basename $f)_$arch.o"
    done
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 $fp "$out"/geometry_$arch.o "$out"/xbox_tag_tables_$arch.o \
        "$out"/xbox_tag_walk_$arch.o "$out"/xbox_blob_$arch.o "$out"/xbox_map_$arch.o "$out"/mapped_file_$arch.o "$out"/cache_endian_$arch.o \
        "$out"/mac_xbox_tag_check_$arch.o -lz -Wl,-dead_strip -o "$out/xbox_tag_check_$arch"
done
for arch in ${MAC_ARCHS:-i386 ppc}; do
    "$out/xbox_tag_check_$arch" "$@" > "$out/result_$arch.txt" || true
    echo "$arch: $(grep -c ': ok' "$out/result_$arch.txt") of $# maps ok"
done
# the coverage (with the vertices and indices) is a little-endian host's: the geometry reads the tags natively
"$out/xbox_tag_check_i386" -c "$@" 2>&1 | grep "coverage" > "$out/coverage_i386.txt" || true
cmp "$out/result_i386.txt" "$out/result_ppc.txt" > /dev/null && echo "same result in both byte orders"
