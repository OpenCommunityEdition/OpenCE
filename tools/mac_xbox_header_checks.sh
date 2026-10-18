#!/bin/sh
# The cache header's words in host order (the PowerPC build refused every map: "does not appear to be a cache file").
# usage: sh tools/mac_xbox_header_checks.sh [map...]   builds for i386 and ppc (run under Rosetta); the outputs must be identical.
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_XBOX_CHECK_OUT:-build/mac-xbox-header}
mkdir -p "$out"
for arch in ${MAC_ARCHS:-i386 ppc}; do
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -std=gnu89 -O1 -Iport/mac -Wno-multichar \
        tools/mac_xbox_header_check.c port/mac/xbox_header.c \
        -Wl,-multiply_defined,suppress -Wl,-dead_strip -o "$out/header_$arch" 2>"$out/build_$arch.txt" || { cat "$out/build_$arch.txt"; exit 1; }
    "$out/header_$arch" "$@" > "$out/header_$arch.txt"
done
cat "$out/header_i386.txt" | tail -3
cmp "$out/header_i386.txt" "$out/header_ppc.txt" && echo "same result in both byte orders"
