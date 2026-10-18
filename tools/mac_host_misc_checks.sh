#!/bin/sh
set -eu
out=${MAC_HOST_MISC_OUT:-build/mac-host-misc}
mkdir -p "$out"
for arch in ppc i386; do
    gcc-4.0 -arch "$arch" -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 port/mac/engine_host_misc.c tools/mac_host_misc_probe.c -o "$out/host_misc_$arch"
done
"$out/host_misc_i386"
