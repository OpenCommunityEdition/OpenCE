#!/bin/sh
set -eu
out=${MAC_INDEX_DECODE_CACHE_OUT:-build/mac-audit/index-decode-cache}
mkdir -p "$out"
mode=${MAC_INDEX_DECODE_CACHE_MODE:-host}
build_only=${BUILD_ONLY:-0}
if [ "$build_only" != 0 ] && [ "$build_only" != 1 ]; then
    echo "BUILD_ONLY must be 0 or 1" >&2
    exit 2
fi

if [ "$mode" = host ]; then
    cc=${CC:-cc}
    sanitize=${SANITIZE:-0}
    flags="-std=gnu89 -O2 -Wall -Wextra -Werror -I. -Iport/mac"
    if [ "$sanitize" = 1 ]; then
        flags="$flags -fsanitize=address,undefined -fno-omit-frame-pointer"
    elif [ "$sanitize" != 0 ]; then
        echo "SANITIZE must be 0 or 1" >&2
        exit 2
    fi
    # CC may contain the host compiler and its normal wrapper arguments.
    # Keep the default path simple while allowing CC="ccache clang".
    # shellcheck disable=SC2086
    $cc $flags port/mac/mac_index_decode_cache.c \
        tools/mac_index_decode_cache_probe.c -o "$out/mac_index_decode_cache_probe"
    if [ "$build_only" != 1 ]; then
        "$out/mac_index_decode_cache_probe" >"$out/runtime.txt" 2>&1
        cat "$out/runtime.txt"
    fi
elif [ "$mode" = mac ]; then
    cc=${MAC_CC:-gcc-4.0}
    sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
    for arch in ${MAC_ARCHS:-ppc i386}; do
        case "$arch" in
            ppc|i386) ;;
            *) echo "unsupported Mac architecture: $arch" >&2; exit 2 ;;
        esac
        fp=
        if [ "$arch" = ppc ]; then fp=-mno-fused-madd; fi
        "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
            -std=gnu89 -O2 $fp -Wall -Wextra -I. -Iport/mac \
            port/mac/mac_index_decode_cache.c tools/mac_index_decode_cache_probe.c \
            -o "$out/mac_index_decode_cache_$arch"
    done
    if [ "$build_only" != 1 ]; then
        run_arch=${MAC_RUN_ARCH:-i386}
        "$out/mac_index_decode_cache_$run_arch" >"$out/runtime-$run_arch.txt" 2>&1
        cat "$out/runtime-$run_arch.txt"
    fi
else
    echo "MAC_INDEX_DECODE_CACHE_MODE must be host or mac" >&2
    exit 2
fi
