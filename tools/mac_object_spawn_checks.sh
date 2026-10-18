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
        -std=gnu89 -O2 $mac_fp -Wall -Wextra -Werror -Iport/mac \
        port/mac/retail_cache.c port/mac/retail_skeleton.c port/mac/retail_object_spawn.c \
        tools/mac_object_spawn_probe.c -o "$mac_dir/object_spawn_$arch"
done
"$mac_dir/object_spawn_i386" "$@"
