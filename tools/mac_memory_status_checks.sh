#!/bin/sh
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_MEMORY_STATUS_OUT:-build/mac-memory-status}
mkdir -p "$out"
for arch in ppc i386; do
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra -Werror -Iport/mac \
        -c port/mac/mac_host_memory.c -o "$out/host_$arch.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -Wall -Wextra -Werror -Iport/mac \
        -c port/mac/engine_handles.c -o "$out/handles_$arch.o"
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 -fms-extensions -fshort-wchar -I. -Isource -Isource/cseries -Isource/math \
        -Iport/mac -idirafter port/linux/include -Iport/include/xdk \
        -include build/mac-audit/mac_engine_semantics.h \
        -Wl,-dead_strip port/mac/engine_memory_status.c tools/mac_memory_status_probe.c \
        "$out/host_$arch.o" "$out/handles_$arch.o" -o "$out/memory_status_$arch"
done
"$out/memory_status_i386"
