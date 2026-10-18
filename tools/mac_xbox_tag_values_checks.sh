#!/bin/sh
# The engine's own reading of real maps' tags, through its structures, on an Intel Mac and (under Rosetta)
# on PowerPC after the conversion: the outputs must be identical. usage: sh tools/mac_xbox_tag_values_checks.sh map...
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_XBOX_CHECK_OUT:-build/mac-xbox-values}
mkdir -p "$out"
engine_flags() {
    echo -DHALO_MAC -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv -I. -Isource -Isource/cseries -Isource/math \
        -Isource/physics -Isource/tag_files -Isource/memory -Isource/cache -Isource/bitmaps -Isource/rasterizer \
        -Isource/models -Isource/structures -Isource/render -Isource/scenario -Isource/objects -Isource/game -Isource/hs \
        -Isource/items -Isource/units -Isource/text -Isource/cutscene -Isource/devices -Isource/ai -Isource/interface \
        -Iport/mac -Iport/linux/game -Iport/linux/src -idirafter port/linux/include -Iport/include/xdk \
        -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_xdk_prefix.h -include port/include/halo_math.h -include port/linux/include/halo_linux_source_fixups.h
}
for arch in ${MAC_ARCHS:-i386 ppc}; do
    fp=; [ "$arch" = ppc ] && fp=-mno-fused-madd
    base="-arch $arch -isysroot $sdk -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp"
    # shellcheck disable=SC2046
    "$cc" $base $(engine_flags) -c tools/mac_xbox_tag_values.c -o "$out/values_$arch.o"
    for f in port/mac/xbox_tag_tables port/mac/xbox_tag_walk port/mac/xbox_blob port/mac/xbox_map port/mac/mapped_file; do
        "$cc" $base -Iport/mac -c $f.c -o "$out/$(basename $f)_$arch.o"
    done
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 $fp -Wl,-multiply_defined,suppress "$out"/values_$arch.o "$out"/xbox_tag_tables_$arch.o \
        "$out"/xbox_tag_walk_$arch.o "$out"/xbox_blob_$arch.o "$out"/xbox_map_$arch.o "$out"/mapped_file_$arch.o -lz -Wl,-dead_strip -o "$out/xbox_tag_values_$arch"
    "$out/xbox_tag_values_$arch" "$@" > "$out/values_$arch.txt"
done
wc -l "$out/values_i386.txt" "$out/values_ppc.txt" | head -2
cmp "$out/values_i386.txt" "$out/values_ppc.txt" && echo "the engine reads the same values in both byte orders"
