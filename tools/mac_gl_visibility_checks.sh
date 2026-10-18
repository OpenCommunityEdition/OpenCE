#!/bin/sh
set -eu
if [ "$(uname -s)" != Darwin ]; then
    echo "mac_gl_visibility_checks.sh requires macOS" >&2
    exit 2
fi
out=${MAC_GL_VISIBILITY_CHECK_DIR:-build/mac-gl-visibility}
cc=${MAC_CC:-gcc-4.0}
sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mkdir -p "$out"
for arch in ppc i386; do
    fp=;if [ "$arch" = ppc ]; then fp=-mno-fused-madd;fi
    "$cc" -arch "$arch" -isysroot "$sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -Wall -Wextra -Werror -DMAC_GL_VISIBILITY_TEST $fp -Iport/mac \
        port/mac/mac_input.c port/mac/mac_window.c port/mac/engine_messages.c \
        port/mac/mac_gl_visibility.c tools/mac_gl_visibility_probe.c \
        -framework Carbon -framework AGL -framework OpenGL -o "$out/mac_gl_visibility_$arch"
done
"$out/mac_gl_visibility_i386"
