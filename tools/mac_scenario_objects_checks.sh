#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    mac_fp=
    if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $mac_fp -Wall -Wextra -Iport/mac -Isource \
        port/mac/retail_cache.c port/mac/native_scenario_objects.c \
        port/mac/native_scenario_blocks.c \
        tools/mac_scenario_blocks_layout_witness.c tools/mac_scenario_objects_probe.c \
        -o "$mac_dir/scenario_objects_$arch"
done
"$mac_dir/scenario_objects_i386" "$@"
