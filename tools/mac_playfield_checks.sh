#!/bin/sh
# Run in staged repository root on the MacBook; probes accept a map argument.
set -eu
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mac_dir=${MAC_CHECK_DIR:-build/mac-audit}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    mac_fp=
    if [ "$arch" = ppc ]; then mac_fp=-mno-fused-madd; fi
    for unit in spawn collision texture; do
        case "$unit" in
            spawn) extra='port/mac/retail_spawns.c'; probe=tools/mac_spawn_probe.c ;;
            collision) extra='port/mac/retail_collision.c port/mac/retail_spawns.c'; probe=tools/mac_collision_probe.c ;;
            texture) extra='port/mac/retail_texture.c'; probe=tools/mac_texture_checks.c ;;
        esac
        "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $mac_fp -Wall -Wextra -Werror -Iport/mac \
            port/mac/retail_cache.c $extra "$probe" -o "$mac_dir/${unit}_$arch"
    done
done
"$mac_dir/spawn_i386" "$@"
"$mac_dir/collision_i386" "$@"
"$mac_dir/texture_i386"
