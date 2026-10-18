#!/bin/sh
# The Xbox texture decoder on real bitmaps, in both byte orders (PowerPC under
# Rosetta on an Intel Mac); the decoded levels' hashes must agree.
# usage: sh tools/mac_xbox_texture_checks.sh map...   (python3 lists the bitmaps; the Intel build Mac's python is 2.6: set MAC_TEXTURE_LISTS to ready lists)
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_XBOX_CHECK_OUT:-build/mac-xbox-texture}
mkdir -p "$out"
python tools/mac_xgpu_texture_decode_view.py "$out/mac_xgpu_texture_decode_view.inc" > /dev/null
for arch in ${MAC_ARCHS:-i386 ppc}; do
    fp=; [ "$arch" = ppc ] && fp=-mno-fused-madd
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -DHALO_MAC \
        -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv -I"$out" -I. -Isource -Isource/cseries -Isource/math \
        -Isource/tag_files -Isource/memory -Isource/rasterizer -Isource/bitmaps -Iport/mac -Iport/linux/src -idirafter port/linux/include -Iport/include/xdk \
        -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_xdk_prefix.h -include port/include/halo_math.h -include port/linux/include/halo_linux_source_fixups.h \
        -c tools/mac_xbox_texture_probe.c -o "$out/probe_$arch.o"
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -Iport/mac -c port/mac/xbox_map.c -o "$out/map_$arch.o"
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -Iport/mac -c port/mac/mapped_file.c -o "$out/mapped_$arch.o"
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 $fp "$out/probe_$arch.o" "$out/map_$arch.o" "$out/mapped_$arch.o" \
        -lz -framework OpenGL -Wl,-dead_strip -o "$out/xbox_texture_probe_$arch"
done
for map in "$@"; do
    name=$(basename "$map" .map)
    list=${MAC_TEXTURE_LISTS:-$out}/$name.list
    [ -f "$list" ] || python3 tools/mac_xbox_texture_list.py "$map" > "$list"
    for arch in ${MAC_ARCHS:-i386 ppc}; do "$out/xbox_texture_probe_$arch" "$map" "$list" > "$out/$name.$arch.txt"; done
    cmp "$out/$name.i386.txt" "$out/$name.ppc.txt" && echo "$name: $(tail -1 "$out/$name.i386.txt"); same hashes in both byte orders"
done
