#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 $fp \
        -fms-extensions -fshort-wchar -I. -Isource -Isource/cseries -Isource/math -Iport/mac \
        -idirafter port/linux/include -Iport/include/xdk -include "$mac_dir/mac_engine_semantics.h" -Wl,-dead_strip \
        port/mac/engine_clock.c port/mac/engine_crt.c tools/mac_engine_clock_probe.c \
        -o "$mac_dir/engine_clock_$arch"
done
"$mac_dir/engine_clock_i386"
