#!/bin/sh
set -eu
if [ "$(uname -s)" != Darwin ]; then
    echo "mac_window_checks.sh requires macOS" >&2
    exit 2
fi
mac_dir=${MAC_WINDOW_CHECK_DIR:-build/mac-window}
mac_cc=${MAC_CC:-gcc-4.0}
mac_sdk=${MAC_SDK:-/Developer/SDKs/MacOSX10.4u.sdk}
mkdir -p "$mac_dir"
for arch in ppc i386; do
    fp=;if [ "$arch" = ppc ]; then fp=-mno-fused-madd;fi
    "$mac_cc" -arch "$arch" -isysroot "$mac_sdk" -mmacosx-version-min=10.4 \
        -std=gnu89 -Wall -Wextra -Werror $fp -Iport/mac \
        port/mac/mac_input.c port/mac/mac_window.c port/mac/engine_messages.c tools/mac_window_probe.c \
        -framework Carbon -framework AGL -framework OpenGL -o "$mac_dir/mac_window_probe_$arch"
done
"$mac_dir/mac_window_probe_i386"
