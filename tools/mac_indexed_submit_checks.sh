#!/bin/sh
set -eu
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
out=${MAC_INDEXED_SUBMIT_OUT:-build/mac-audit/indexed-submit}
build_only=${BUILD_ONLY:-0}
if [ "$build_only" != 0 ] && [ "$build_only" != 1 ]; then
    echo "BUILD_ONLY must be 0 or 1" >&2
    exit 2
fi
mkdir -p "$out"
for arch in ${MAC_ARCHS:-ppc i386}; do
    case "$arch" in ppc|i386) ;; *) echo "unsupported Mac architecture: $arch" >&2; exit 2 ;; esac
    fp=
    if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -O2 $fp -Wall -Wextra -I. -Iport/mac \
        port/mac/mac_glsl120.c port/mac/mac_index_decode_cache.c \
        tools/mac_indexed_submit_probe.c -framework AGL -framework OpenGL \
        -o "$out/mac_indexed_submit_$arch"
done
if [ "$build_only" != 1 ]; then
    run_arch=${MAC_RUN_ARCH:-i386}
    "$out/mac_indexed_submit_$run_arch" >"$out/runtime-$run_arch.txt" 2>&1
    cat "$out/runtime-$run_arch.txt"
fi
