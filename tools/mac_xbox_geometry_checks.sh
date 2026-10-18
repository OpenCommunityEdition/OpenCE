#!/bin/sh
# Converts and checks the raw geometry of real Xbox maps for both byte orders.
# The PowerPC build runs under Rosetta on an Intel Mac (the conversion and the
# checks need no game session). usage: sh tools/mac_xbox_geometry_checks.sh map...
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_XBOX_CHECK_OUT:-build/mac-xbox-geometry}
mkdir -p "$out"
for arch in ${MAC_ARCHS:-i386 ppc}; do
    fp=; [ "$arch" = ppc ] && fp=-mno-fused-madd
    # the engine's own flags for the unit that reads the engine's structures
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -DHALO_MAC \
        -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv -I. -Isource -Isource/cseries -Isource/math \
        -Isource/physics -Isource/tag_files -Isource/memory -Isource/cache -Isource/bitmaps -Isource/rasterizer \
        -Isource/models -Isource/structures -Isource/render -Isource/scenario -Isource/objects -Isource/game \
        -Iport/mac -Iport/linux/game -Iport/linux/src -idirafter port/linux/include -Iport/include/xdk \
        -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_xdk_prefix.h -include port/include/halo_math.h \
        -include port/linux/include/halo_linux_source_fixups.h \
        -c port/mac/xbox_geometry.c -o "$out/xbox_geometry_$arch.o"
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp -Wall -Iport/mac \
        tools/mac_xbox_geometry_probe.c "$out/xbox_geometry_$arch.o" port/mac/xbox_map.c port/mac/mapped_file.c \
        -lz -Wl,-dead_strip -o "$out/xbox_geometry_probe_$arch"
    if [ "$arch" = i386 ]; then
        # the Intel run walks the tags and lists the buffers it converted
        HALO_MAC_XBOX_FORCE_SWAP=1 MAC_XBOX_GEOMETRY_LIST="$out/buffers.txt" "$out/xbox_geometry_probe_$arch" "$@" > "$out/result_$arch.txt"
    else
        # the PowerPC run (under Rosetta) converts and checks that list: its tags are not swapped yet, so it cannot walk them
        MAC_XBOX_GEOMETRY_MODE=apply MAC_XBOX_GEOMETRY_LIST="$out/buffers.txt" "$out/xbox_geometry_probe_$arch" "$@" > "$out/result_$arch.txt"
    fi
    echo "$arch: $(wc -l < "$out/result_$arch.txt") maps"
done
cmp "$out/result_i386.txt" "$out/result_ppc.txt" && echo "same result in both byte orders"
