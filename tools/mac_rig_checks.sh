#!/bin/sh
# Standalone bounded native skeleton/animation readers; no engine overlay casts.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    mac_fp=
    if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
    for probe in skeleton animation animation_mutation; do
        reader=animation
        if [ "$probe" = skeleton ]; then reader=skeleton; fi
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $mac_fp -Wall -Wextra -Werror -Iport/mac \
            port/mac/retail_cache.c "port/mac/retail_$reader.c" \
            "tools/mac_${probe}_probe.c" -o "$mac_dir/${probe}_$arch"
    done
done
for probe in skeleton animation animation_mutation; do
    "$mac_dir/${probe}_i386" "$@"
done
