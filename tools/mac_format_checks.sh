#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ${MAC_ARCHS:-ppc i386}; do
    fp=; if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -Iport/mac -c port/mac/engine_format.c -o "$mac_dir/engine_format_$arch.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -Iport/mac tools/mac_format_probe.c "$mac_dir/engine_format_$arch.o" -o "$mac_dir/format_$arch"
done
