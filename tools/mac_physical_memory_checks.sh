#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
if [ ! -f "$mac_dir/mac_engine_semantics.h" ]; then
    echo "Generate mac_engine_semantics.h with tools/linux_msvc_semantics.py first" >&2
    exit 1
fi
for arch in ppc i386; do
    mac_fp=
    if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -fms-extensions -fshort-wchar -fno-strict-aliasing -fwrapv \
        -I. -Isource -Isource/cseries -Isource/math -Isource/physics -Isource/tag_files \
        -Isource/memory -Isource/input -Isource/game -Isource/objects \
        -Iport/mac -Iport/linux/game -Iport/linux/include -Iport/include/xdk \
        -include port/linux/include/halo_port_limits.h -include port/linux/include/halo_port_capacity.h \
        -include port/mac/engine_xdk_prefix.h -include port/include/halo_math.h \
        -include "$mac_dir/mac_engine_semantics.h" \
        port/mac/engine_physical_memory.c tools/mac_physical_memory_probe.c \
        -Wl,-dead_strip -o "$mac_dir/physical_memory_$arch"
done
"$mac_dir/physical_memory_i386"
