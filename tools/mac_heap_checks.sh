#!/bin/sh
set -eu
out=${MAC_HEAP_OUT:-build/mac-heap}
mkdir -p "$out"
for arch in ppc i386; do
    fp=;if [ "$arch" = ppc ];then fp=-mno-fused-madd;fi
    gcc-4.0 -arch "$arch" -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -fshort-wchar -fms-extensions -ffunction-sections -fdata-sections \
        -I. -Isource -Isource/cseries -Isource/math -Iport/mac \
        -include port/mac/engine_prefix.h -include build/mac-audit/mac_engine_semantics.h \
        port/mac/engine_services.c port/mac/engine_heap.c tools/mac_heap_probe.c \
        -Wl,-dead_strip -o "$out/heap_$arch"
done
"$out/heap_i386"
