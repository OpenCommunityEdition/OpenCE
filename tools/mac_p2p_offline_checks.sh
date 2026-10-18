#!/bin/sh
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_P2P_OFFLINE_OUT:-build/mac-audit/p2p-offline}
mkdir -p "$out"
for arch in ppc i386; do
    flags="-arch $arch -isysroot $sdk -mmacosx-version-min=10.4"
    "$cc" $flags -std=gnu89 -O2 -ffunction-sections -fdata-sections \
        -Iport/linux/src -Iport/third_party/monocypher -c \
        port/mac/engine_p2p_offline.c -o "$out/offline_$arch.o"
    "$cc" $flags -std=gnu99 -O2 -ffunction-sections -fdata-sections \
        -Iport/third_party/monocypher -c \
        port/third_party/monocypher/monocypher.c -o "$out/monocypher_$arch.o"
    "$cc" $flags -std=gnu89 -O2 -Iport/linux/src -c \
        tools/mac_p2p_offline_probe.c -o "$out/probe_$arch.o"
    "$cc" $flags "$out/offline_$arch.o" "$out/monocypher_$arch.o" \
        "$out/probe_$arch.o" -o "$out/probe_$arch"
done
"$out/probe_i386"
