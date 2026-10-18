#!/bin/sh
# Opens and checks Xbox (version 5) maps with the Mac port's container reader
# (port/mac/xbox_map.c), for both byte orders. The PowerPC build runs under
# Rosetta on an Intel Mac (the container code is endian-explicit; a game
# session is not run that way). usage: sh tools/mac_xbox_map_checks.sh map...
set -eu
if [ "$#" -lt 1 ]; then
    echo "usage: $0 map.map..." >&2
    exit 2
fi
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_XBOX_CHECK_OUT:-build/mac-xbox-map-check}
mkdir -p "$out"
for arch in ${MAC_ARCHS:-i386 ppc}; do
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 -std=gnu89 -O2 \
        -Wall -Wextra -Werror -Iport/mac tools/mac_xbox_map_check.c port/mac/xbox_map.c \
        port/mac/mapped_file.c -lz -o "$out/xbox_map_check_$arch"
    "$out/xbox_map_check_$arch" "$@" > "$out/result_$arch.txt"
    echo "$arch: $(wc -l < "$out/result_$arch.txt") maps checked"
done
if [ -f "$out/result_i386.txt" ] && [ -f "$out/result_ppc.txt" ]; then
    cmp "$out/result_i386.txt" "$out/result_ppc.txt" && echo "same result in both byte orders"
fi
